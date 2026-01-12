/*
 * src/Libs/WaveFactory/FileFormatMIDI.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <type_traits>
#include <vector>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Processor.hpp"
#include "Synthesizer.hpp"
#include "Wave.hpp"

/* TinySoundFont inclusion for SoundFont rendering. */
#include "tsf.h"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Class for reading MIDI files and converting them to synthesized audio.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @extends EmEn::Libs::WaveFactory::FileFormatInterface The base IO class.
	 * @note This is a basic implementation supporting Note On/Off events and tempo changes.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatMIDI final : public FileFormatInterface< precision_t >
	{
		public:

			/**
			 * @brief Constructs a MIDI format IO with default sample rate.
			 * @param frequency The sample rate to use for generation. Default 48kHz.
			 */
			explicit
			FileFormatMIDI (Frequency frequency = Frequency::PCM48000Hz) noexcept
				: m_frequency{frequency}
			{

			}

			/**
			 * @brief Sets the sample rate for audio generation.
			 * @param frequency The sample rate.
			 * @return void
			 */
			void
			setFrequency (Frequency frequency) noexcept
			{
				m_frequency = frequency;
			}

			/**
			 * @brief Returns the current sample rate.
			 * @return Frequency
			 */
			[[nodiscard]]
			Frequency
			frequency () const noexcept
			{
				return m_frequency;
			}

			/**
			 * @brief Sets a SoundFont for high-quality sample-based rendering.
			 * @param soundfont Pointer to a TinySoundFont handle, or nullptr for additive synthesis.
			 * @note The caller retains ownership of the tsf handle.
			 */
			void
			setSoundfont (tsf * soundfont) noexcept
			{
				m_soundfont = soundfont;
			}

			/**
			 * @brief Returns the current SoundFont handle.
			 * @return tsf* Pointer to the SoundFont, or nullptr if none set.
			 */
			[[nodiscard]]
			tsf *
			soundfont () const noexcept
			{
				return m_soundfont;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Wave< precision_t > & wave) noexcept override
			{
				std::ifstream file(filepath, std::ios::binary);

				if ( !file.is_open() )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] readFile(), failed to open '" << filepath << "' !\n";

					return false;
				}

				/* Parse MIDI header. */
				MIDIHeader header;

				if ( !this->parseHeader(file, header) )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] readFile(), invalid MIDI header in '" << filepath << "' !\n";

					return false;
				}

				/* Parse all tracks and collect notes, control events, and tempo events. */
				std::vector< MIDINote > notes;
				std::vector< ControlEvent > controlEvents;
				std::vector< TempoEvent > tempoEvents;
				std::array< ChannelState, 16 > channelStates{};

				/* Pre-allocate vectors to avoid reallocations during parsing. */
				notes.reserve(header.trackCount * 100);		/* Typical: ~100 notes per track. */
				controlEvents.reserve(header.trackCount * 50); /* Fewer control events. */
				tempoEvents.reserve(16);					   /* Rarely more than a few tempo changes. */

				for ( uint16_t trackIndex = 0; trackIndex < header.trackCount; ++trackIndex )
				{
					if ( !this->parseTrack(file, notes, controlEvents, tempoEvents, channelStates, trackIndex) )
					{
						std::cerr << "[WaveFactory::FileFormatMIDI] readFile(), failed to parse track " << trackIndex << " in '" << filepath << "' !\n";

						return false;
					}
				}

				/* Sort tempo events by tick for correct tempo mapping. */
				std::sort(tempoEvents.begin(), tempoEvents.end(), [] (const TempoEvent & a, const TempoEvent & b) {
					return a.tick < b.tick;
				});

				/* Ensure we have at least a default tempo if the file doesn't specify one. */
				if ( tempoEvents.empty() || tempoEvents[0].tick > 0 )
				{
					tempoEvents.insert(tempoEvents.begin(), {0, 500000});
				}

				if ( notes.empty() )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] readFile(), no notes found in '" << filepath << "' !\n";

					return false;
				}

				/* Sort control events by time for efficient lookup during rendering. */
				std::sort(controlEvents.begin(), controlEvents.end(), [] (const ControlEvent & a, const ControlEvent & b) {
					return a.tick < b.tick;
				});

				/* Render notes to wave. */
				if ( !this->renderToWave(wave, notes, controlEvents, tempoEvents, header, channelStates) )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] readFile(), failed to render audio from '" << filepath << "' !\n";

					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & /*filepath*/, const Wave< precision_t > & /*wave*/) const noexcept override
			{
				/* NOTE: Converting audio back to MIDI would require pitch detection and note segmentation,
				 * which is beyond the scope of this basic implementation. */
				std::cerr << "[WaveFactory::FileFormatMIDI] writeFile() is not supported ! MIDI format is read-only.\n";

				return false;
			}

		private:

			/**
			 * @brief MIDI file header structure.
			 */
			struct MIDIHeader
			{
				uint16_t format{0};
				uint16_t trackCount{0};
				uint16_t division{480};		/* Ticks per quarter note. */
			};

			/**
			 * @brief Represents a tempo change event in the MIDI file.
			 */
			struct TempoEvent
			{
				uint32_t tick{0};			  /* Tick position of the tempo change. */
				uint32_t tempo{500000};		/* Microseconds per quarter note. */
			};

			/**
			 * @brief Represents a parsed MIDI note with timing.
			 */
			struct MIDINote
			{
				uint32_t startTick{0};
				uint32_t endTick{0};
				uint8_t noteNumber{0};
				uint8_t velocity{0};
				uint8_t channel{0};
				uint16_t trackIndex{0};
			};

			/**
			 * @brief Stores state per MIDI channel.
			 * @note Pan is 0-127 where 0=left, 64=center, 127=right.
			 * @note Program is 0-127 following General MIDI instrument mapping.
			 */
			struct ChannelState
			{
				/* Ordered by size (largest first) to minimize padding. */
				float pitchBendRange{2.0F}; /* Pitch bend range in semitones (default ±2). */
				int16_t pitchBend{0};	   /* Pitch bend value (-8192 to +8191). */
				uint8_t pan{64};			/* Default center (CC#10). */
				uint8_t program{0};		 /* Default piano (Acoustic Grand). */
				uint8_t modulation{0};	  /* Modulation wheel (CC#1). */
				uint8_t expression{127};	/* Expression controller (CC#11), default max. */
				uint8_t volume{100};		/* Channel volume (CC#7). */
				uint8_t portamentoTime{0};  /* Portamento time (CC#5), 0-127. */
				uint8_t filterCutoff{127};  /* Filter cutoff (CC#74), default fully open. */
				uint8_t filterResonance{0}; /* Filter resonance (CC#71), default no resonance. */
				uint8_t tremoloDepth{0};	/* Tremolo depth (CC#92), default off. */
				bool sustainPedal{false};   /* Sustain pedal state (CC#64). */
				bool portamentoOn{false};   /* Portamento on/off (CC#65). */
			};

			/**
			 * @brief Represents a MIDI control change event with timing.
			 * @note Used to track controller changes over time for accurate playback.
			 */
			struct ControlEvent
			{
				/**
				 * @brief Type of control event.
				 */
				enum class Type : uint8_t
				{
					PitchBend,
					Pan,
					Modulation,
					Expression,
					Volume,
					Sustain,
					PortamentoTime,
					PortamentoSwitch,
					FilterCutoff,
					FilterResonance,
					Tremolo,
					ChannelPressure,   /* Aftertouch affecting whole channel. */
					PolyKeyPressure,   /* Aftertouch affecting specific note. */
					ProgramChange,	 /* Instrument change during playback. */
					RawMidiCC		  /* Pass-through for CC handled natively by TSF. */
				};

				uint32_t tick{0};	  /* Event time in ticks. */
				uint8_t channel{0};	/* MIDI channel (0-15). */
				Type type{Type::PitchBend};
				uint8_t controller{0}; /* Raw CC number (for RawMidiCC type). */
				int16_t value{0};	  /* Event value. */
			};

			/** @brief Alias to Synthesizer types for convenience. */
			using Synth = Synthesizer< precision_t >;
			using InstrumentFamily = Synth::InstrumentFamily;
			using FilterState = Synth::FilterState;

			/**
			 * @brief Reads a big-endian 16-bit value from stream.
			 * @param stream The input stream.
			 * @return uint16_t
			 */
			[[nodiscard]]
			static uint16_t
			readUInt16BE (std::istream & stream) noexcept
			{
				uint8_t bytes[2];
				stream.read(reinterpret_cast< char * >(bytes), 2);

				return static_cast< uint16_t >((bytes[0] << 8) | bytes[1]);
			}

			/**
			 * @brief Reads a big-endian 32-bit value from stream.
			 * @param stream The input stream.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static uint32_t
			readUInt32BE (std::istream & stream) noexcept
			{
				uint8_t bytes[4];
				stream.read(reinterpret_cast< char * >(bytes), 4);

				return (static_cast< uint32_t >(bytes[0]) << 24) | (static_cast< uint32_t >(bytes[1]) << 16) | (static_cast< uint32_t >(bytes[2]) << 8) | static_cast< uint32_t >(bytes[3]);
			}

			/**
			 * @brief Reads a MIDI variable-length quantity.
			 * @param stream The input stream.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static uint32_t
			readVariableLength (std::istream & stream) noexcept
			{
				uint32_t value = 0;
				uint8_t byte;

				do
				{
					const int rawByte = stream.get();

					/* Handle EOF to prevent infinite loop on truncated files. */
					if ( rawByte == std::istream::traits_type::eof() || !stream.good() ) [[unlikely]]
					{
						break;
					}

					byte = static_cast< uint8_t >(rawByte);
					value = (value << 7) | (byte & 0x7F);
				}
				while ( byte & 0x80 );

				return value;
			}

			/**
			 * @brief Parses the MIDI file header.
			 * @param stream The input stream.
			 * @param header The header structure to fill.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			parseHeader (std::istream & stream, MIDIHeader & header) noexcept
			{
				/* Read chunk type (should be "MThd"). */
				char magic[4];
				stream.read(magic, 4);

				if ( std::strncmp(magic, "MThd", 4) != 0 )
				{
					return false;
				}

				/* Read header length (should be 6). */
				const auto length = readUInt32BE(stream);

				if ( length != 6 )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] parseHeader(), unexpected header length: " << length << " !\n";

					return false;
				}

				/* Read header data. */
				header.format = readUInt16BE(stream);
				header.trackCount = readUInt16BE(stream);
				header.division = readUInt16BE(stream);

				/* Check for SMPTE time division (not supported). */
				if ( header.division & 0x8000 )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] parseHeader(), SMPTE time division not supported !\n";

					return false;
				}

				return true;
			}

			/**
			 * @brief Parses a single MIDI track.
			 * @param stream The input stream.
			 * @param notes The vector to append parsed notes to.
			 * @param controlEvents The vector to append control events to.
			 * @param tempoEvents The vector to append tempo events to.
			 * @param channelStates The channel state array to update (pan, etc.).
			 * @param trackIndex The index of this track.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			parseTrack (std::istream & stream, std::vector< MIDINote > & notes, std::vector< ControlEvent > & controlEvents, std::vector< TempoEvent > & tempoEvents, std::array< ChannelState, 16 > & channelStates, uint16_t trackIndex) noexcept
			{
				/* Read chunk type (should be "MTrk"). */
				char magic[4];
				stream.read(magic, 4);

				if ( std::strncmp(magic, "MTrk", 4) != 0 )
				{
					return false;
				}

				/* Read track length. */
				const auto trackLength = readUInt32BE(stream);
				const auto trackEnd = static_cast< std::streampos >(stream.tellg()) + static_cast< std::streamoff >(trackLength);

				/* Track active notes (for Note On/Off matching). */
				std::unordered_map< uint16_t, MIDINote > activeNotes;
				activeNotes.reserve(128);  /* Max 128 MIDI notes active simultaneously. */

				uint32_t currentTick = 0;
				uint8_t runningStatus = 0;

				while ( stream.tellg() < trackEnd && stream.good() )
				{
					/* Read delta time. */
					const auto deltaTime = readVariableLength(stream);
					currentTick += deltaTime;

					/* Read event. */
					auto status = static_cast< uint8_t >(stream.peek());

					/* Handle running status (common in MIDI files). */
					if ( status < 0x80 ) [[likely]]
					{
						status = runningStatus;
					}
					else
					{
						stream.get();
						runningStatus = status;
					}

					const uint8_t eventType = status & 0xF0;
					const uint8_t channel = status & 0x0F;

					switch ( eventType )
					{
						case 0x80: /* Note Off */
						{
							const auto noteNumber = static_cast< uint8_t >(stream.get());
							stream.get(); /* Velocity (ignored for Note Off). */

							const uint16_t key = (static_cast< uint16_t >(channel) << 8) | noteNumber;
							auto iterator = activeNotes.find(key);

							if ( iterator != activeNotes.end() )
							{
								iterator->second.endTick = currentTick;
								notes.push_back(iterator->second);
								activeNotes.erase(iterator);
							}

							break;
						}

						case 0x90: /* Note On */
						{
							const auto noteNumber = static_cast< uint8_t >(stream.get());
							const auto velocity = static_cast< uint8_t >(stream.get());

							const uint16_t key = (static_cast< uint16_t >(channel) << 8) | noteNumber;

							if ( velocity == 0 ) [[unlikely]]
							{
								/* Note On with velocity 0 is equivalent to Note Off. */
								auto iterator = activeNotes.find(key);

								if ( iterator != activeNotes.end() )
								{
									iterator->second.endTick = currentTick;
									notes.push_back(iterator->second);
									activeNotes.erase(iterator);
								}
							}
							else [[likely]]
							{
								MIDINote note;
								note.startTick = currentTick;
								note.noteNumber = noteNumber;
								note.velocity = velocity;
								note.channel = channel;
								note.trackIndex = trackIndex;
								activeNotes[key] = note;
							}

							break;
						}

						case 0xA0: /* Polyphonic Key Pressure (Aftertouch per note) */
						{
							const auto noteNumber = static_cast< uint8_t >(stream.get());
							const auto pressure = static_cast< uint8_t >(stream.get());

							ControlEvent event;
							event.tick = currentTick;
							event.channel = channel;
							event.type = ControlEvent::Type::PolyKeyPressure;
							event.controller = noteNumber;  /* Store note number in controller field. */
							event.value = static_cast< int16_t >(pressure);
							controlEvents.push_back(event);

							break;
						}

						case 0xB0: /* Control Change */
						{
							const auto controller = static_cast< uint8_t >(stream.get());
							const auto value = static_cast< uint8_t >(stream.get());

							switch ( controller )
							{
								case 1: /* CC#1: Modulation wheel. */
								{
									channelStates[channel].modulation = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Modulation;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 5: /* CC#5: Portamento time. */
								{
									channelStates[channel].portamentoTime = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::PortamentoTime;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 7: /* CC#7: Channel volume. */
								{
									channelStates[channel].volume = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Volume;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 10: /* CC#10: Pan (0=left, 64=center, 127=right). */
								{
									channelStates[channel].pan = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Pan;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 11: /* CC#11: Expression controller. */
								{
									channelStates[channel].expression = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Expression;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 64: /* CC#64: Sustain pedal. */
								{
									channelStates[channel].sustainPedal = (value >= 64);

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Sustain;
									event.value = (value >= 64) ? 1 : 0;
									controlEvents.push_back(event);

									break;
								}

								case 65: /* CC#65: Portamento on/off. */
								{
									channelStates[channel].portamentoOn = (value >= 64);

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::PortamentoSwitch;
									event.value = (value >= 64) ? 1 : 0;
									controlEvents.push_back(event);

									break;
								}

								case 71: /* CC#71: Filter resonance (Timbre/Harmonic Intensity). */
								{
									channelStates[channel].filterResonance = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::FilterResonance;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 74: /* CC#74: Filter cutoff (Brightness). */
								{
									channelStates[channel].filterCutoff = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::FilterCutoff;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								case 92: /* CC#92: Tremolo depth (Effect 2 Depth). */
								{
									channelStates[channel].tremoloDepth = value;

									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::Tremolo;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								/* TSF-native controllers: Bank Select, RPN, Data Entry, LSB controllers, All Notes Off. */
								case 0:   /* CC#0: Bank Select MSB. */
								case 6:   /* CC#6: Data Entry MSB (for RPN). */
								case 32:  /* CC#32: Bank Select LSB. */
								case 38:  /* CC#38: Data Entry LSB (for RPN). */
								case 39:  /* CC#39: Volume LSB. */
								case 42:  /* CC#42: Pan LSB. */
								case 43:  /* CC#43: Expression LSB. */
								case 98:  /* CC#98: NRPN LSB. */
								case 99:  /* CC#99: NRPN MSB. */
								case 100: /* CC#100: RPN LSB. */
								case 101: /* CC#101: RPN MSB. */
								case 120: /* CC#120: All Sound Off. */
								case 121: /* CC#121: Reset All Controllers. */
								case 123: /* CC#123: All Notes Off. */
								{
									ControlEvent event;
									event.tick = currentTick;
									event.channel = channel;
									event.type = ControlEvent::Type::RawMidiCC;
									event.controller = controller;
									event.value = static_cast< int16_t >(value);
									controlEvents.push_back(event);

									break;
								}

								default:
									break;
							}

							break;
						}

						case 0xC0: /* Program Change */
						{
							const auto program = static_cast< uint8_t >(stream.get());
							channelStates[channel].program = program;

							ControlEvent event;
							event.tick = currentTick;
							event.channel = channel;
							event.type = ControlEvent::Type::ProgramChange;
							event.value = static_cast< int16_t >(program);
							controlEvents.push_back(event);

							break;
						}

						case 0xD0: /* Channel Pressure (Aftertouch for whole channel) */
						{
							const auto pressure = static_cast< uint8_t >(stream.get());

							ControlEvent event;
							event.tick = currentTick;
							event.channel = channel;
							event.type = ControlEvent::Type::ChannelPressure;
							event.value = static_cast< int16_t >(pressure);
							controlEvents.push_back(event);

							break;
						}

						case 0xE0: /* Pitch Bend */
						{
							const auto lsb = static_cast< uint8_t >(stream.get());
							const auto msb = static_cast< uint8_t >(stream.get());

							/* Combine LSB and MSB into 14-bit value (0-16383), center is 8192. */
							const int16_t bendValue = static_cast< int16_t >((static_cast< uint16_t >(msb) << 7) | lsb) - 8192;
							channelStates[channel].pitchBend = bendValue;

							ControlEvent event;
							event.tick = currentTick;
							event.channel = channel;
							event.type = ControlEvent::Type::PitchBend;
							event.value = bendValue;
							controlEvents.push_back(event);

							break;
						}

						case 0xF0: /* System / Meta events */
						{
							if ( status == 0xFF )
							{
								/* Meta event. */
								const auto metaType = static_cast< uint8_t >(stream.get());
								const auto metaLength = readVariableLength(stream);

								if ( metaType == 0x2F )
								{
									/* End of Track - exit parsing loop. */
									stream.seekg(metaLength, std::ios::cur);

									/* Close any remaining active notes. */
									for ( auto & [key, note] : activeNotes )
									{
										note.endTick = currentTick;
										notes.push_back(note);
									}

									return true;
								}
								else if ( metaType == 0x51 && metaLength == 3 )
								{
									/* Tempo change - store as event for accurate timing. */
									uint8_t tempoBytes[3];
									stream.read(reinterpret_cast< char * >(tempoBytes), 3);
									const uint32_t tempo = (static_cast< uint32_t >(tempoBytes[0]) << 16) |
														   (static_cast< uint32_t >(tempoBytes[1]) << 8) |
														   static_cast< uint32_t >(tempoBytes[2]);
									tempoEvents.push_back({currentTick, tempo});
								}
								else
								{
									/* Skip other meta events. */
									stream.seekg(metaLength, std::ios::cur);
								}
							}
							else if ( status == 0xF0 || status == 0xF7 )
							{
								/* SysEx event. */
								const auto sysexLength = readVariableLength(stream);
								stream.seekg(sysexLength, std::ios::cur);
							}

							runningStatus = 0;

							break;
						}

						default:
							break;
					}
				}

				/* Close any remaining active notes at track end. */
				for ( auto & [key, note] : activeNotes )
				{
					note.endTick = currentTick;
					notes.push_back(note);
				}

				return true;
			}

			/**
			 * @brief Tracks control values for a channel during rendering with O(1) updates.
			 */
			struct ChannelControlCache
			{
				int16_t pitchBend{0};
				int16_t pan{64};
				int16_t modulation{0};
				int16_t expression{127};
				int16_t volume{100};
				int16_t portamentoTime{0};
				int16_t filterCutoff{127};
				int16_t filterResonance{0};
				int16_t tremoloDepth{0};
				bool sustain{false};
				bool portamentoOn{false};
			};

			/**
			 * @brief Builds an index of control events per channel for fast lookup.
			 * @param controlEvents The sorted vector of all control events.
			 * @return std::array of vectors, one per channel, containing indices into controlEvents.
			 */
			[[nodiscard]]
			static
			std::array< std::vector< size_t >, 16 >
			buildChannelEventIndex (const std::vector< ControlEvent > & controlEvents) noexcept
			{
				std::array< std::vector< size_t >, 16 > index{};

				for ( size_t i = 0; i < controlEvents.size(); ++i )
				{
					index[controlEvents[i].channel].push_back(i);
				}

				return index;
			}

			/**
			 * @brief Updates the control cache by processing events up to the given tick.
			 * @param controlEvents The sorted vector of control events.
			 * @param channelIndex The indices of events for this channel.
			 * @param cache The cache to update.
			 * @param searchIndex The current search position (updated in place).
			 * @param tick The target tick.
			 */
			static
			void
			updateControlCache (const std::vector< ControlEvent > & controlEvents, const std::vector< size_t > & channelIndex, ChannelControlCache & cache, size_t & searchIndex, uint32_t tick) noexcept
			{
				while ( searchIndex < channelIndex.size() )
				{
					const auto & event = controlEvents[channelIndex[searchIndex]];

					if ( event.tick > tick )
					{
						break;
					}

					switch ( event.type )
					{
						case ControlEvent::Type::PitchBend:
							cache.pitchBend = event.value;
							break;

						case ControlEvent::Type::Pan:
							cache.pan = event.value;
							break;

						case ControlEvent::Type::Modulation:
							cache.modulation = event.value;
							break;

						case ControlEvent::Type::Expression:
							cache.expression = event.value;
							break;

						case ControlEvent::Type::Volume:
							cache.volume = event.value;
							break;

						case ControlEvent::Type::Sustain:
							cache.sustain = (event.value != 0);
							break;

						case ControlEvent::Type::PortamentoTime:
							cache.portamentoTime = event.value;
							break;

						case ControlEvent::Type::PortamentoSwitch:
							cache.portamentoOn = (event.value != 0);
							break;

						case ControlEvent::Type::FilterCutoff:
							cache.filterCutoff = event.value;
							break;

						case ControlEvent::Type::FilterResonance:
							cache.filterResonance = event.value;
							break;

						case ControlEvent::Type::Tremolo:
							cache.tremoloDepth = event.value;
							break;

						case ControlEvent::Type::RawMidiCC:
							/* Ignored in additive synthesis - only meaningful for SF2 rendering. */
							break;

						case ControlEvent::Type::ChannelPressure:
							/* Channel aftertouch can modulate expression for added dynamics. */
							cache.expression = std::max(cache.expression, event.value);
							break;

						case ControlEvent::Type::PolyKeyPressure:
							/* Per-note aftertouch - harder to apply in this context, skip for now. */
							break;

						case ControlEvent::Type::ProgramChange:
							/* Program change only affects SF2 rendering, not additive synthesis. */
							break;
					}

					++searchIndex;
				}
			}

			/**
			 * @brief Gets the ADSR envelope parameters for an instrument family.
			 * @param family The instrument family.
			 * @param noteDuration The duration of the note in seconds.
			 * @param attack Output attack time.
			 * @param decay Output decay time.
			 * @param sustain Output sustain level.
			 * @param release Output release time.
			 */
			static void
			getADSRForFamily (InstrumentFamily family, float noteDuration, float & attack, float & decay, float & sustain, float & release) noexcept
			{
				switch ( family )
				{
					case InstrumentFamily::Piano:
					case InstrumentFamily::Guitar:
					case InstrumentFamily::Chromatic:
						/* Plucked/struck: fast attack, quick decay. */
						attack = 0.005F;
						decay = std::min(0.1F, noteDuration * 0.3F);
						sustain = 0.4F;
						release = std::min(0.2F, noteDuration * 0.3F);
						break;

					case InstrumentFamily::Organ:
					case InstrumentFamily::SynthLead:
						/* Sustained: instant attack, no decay. */
						attack = 0.005F;
						decay = 0.01F;
						sustain = 0.9F;
						release = std::min(0.05F, noteDuration * 0.1F);
						break;

					case InstrumentFamily::Strings:
					case InstrumentFamily::SynthPad:
					case InstrumentFamily::Ensemble:
						/* Smooth: slow attack, long release. */
						attack = std::min(0.1F, noteDuration * 0.2F);
						decay = std::min(0.1F, noteDuration * 0.2F);
						sustain = 0.7F;
						release = std::min(0.3F, noteDuration * 0.4F);
						break;

					case InstrumentFamily::Brass:
					case InstrumentFamily::Reed:
					case InstrumentFamily::Pipe:
						/* Wind: moderate attack. */
						attack = std::min(0.03F, noteDuration * 0.1F);
						decay = std::min(0.05F, noteDuration * 0.15F);
						sustain = 0.8F;
						release = std::min(0.1F, noteDuration * 0.2F);
						break;

					case InstrumentFamily::Bass:
						/* Bass: punchy attack, moderate sustain. */
						attack = 0.01F;
						decay = std::min(0.08F, noteDuration * 0.2F);
						sustain = 0.6F;
						release = std::min(0.15F, noteDuration * 0.25F);
						break;

					default:
						/* Default envelope. */
						attack = std::min(0.01F, noteDuration * 0.1F);
						decay = std::min(0.05F, noteDuration * 0.2F);
						sustain = 0.7F;
						release = std::min(0.1F, noteDuration * 0.3F);
						break;
				}
			}

			/**
			 * @brief Renders parsed MIDI notes using TinySoundFont for high-quality sample-based output.
			 * @param wave The destination wave (will be stereo).
			 * @param notes The parsed notes.
			 * @param controlEvents The control change events (pitch bend, modulation, etc.).
			 * @param tempoEvents The tempo change events (sorted by tick).
			 * @param header The MIDI header for timing info.
			 * @param channelStates The channel states containing pan and program values.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			renderWithSoundfont (Wave< precision_t > & wave, const std::vector< MIDINote > & notes, const std::vector< ControlEvent > & controlEvents, const std::vector< TempoEvent > & tempoEvents, const MIDIHeader & header, const std::array< ChannelState, 16 > & channelStates) noexcept
			{
				const auto sampleRate = static_cast< uint32_t >(m_frequency);

				/* Configure TSF for stereo interleaved output. */
				tsf_set_output(m_soundfont, TSF_STEREO_INTERLEAVED, static_cast< int >(sampleRate), 0.0F);
				tsf_reset(m_soundfont);

				/* Pre-allocate voices for thread safety and performance.
				 * 256 voices should be enough for most complex MIDI files. */
				tsf_set_max_voices(m_soundfont, 256);

				/* Set initial channel states (program, pan, volume). */
				for ( uint8_t channel = 0; channel < 16; ++channel )
				{
					const auto & state = channelStates[channel];

					/* Set instrument program (bank 128 for channel 9 = drums). */
					if ( channel == 9 )
					{
						tsf_channel_set_bank_preset(m_soundfont, channel, 128, state.program);
					}
					else
					{
						tsf_channel_set_bank_preset(m_soundfont, channel, 0, state.program);
					}

					/* Set pan (TSF uses 0.0 = left, 0.5 = center, 1.0 = right). */
					tsf_channel_set_pan(m_soundfont, channel, static_cast< float >(state.pan) / 127.0F);

					/* Set volume. */
					tsf_channel_set_volume(m_soundfont, channel, static_cast< float >(state.volume) / 127.0F);
				}

				/* Find the last note end to determine total duration. */
				uint32_t maxEndTick = 0;

				for ( const auto & note : notes )
				{
					maxEndTick = std::max(maxEndTick, note.endTick);
				}

				/* Add a small tail for release. */
				const uint32_t tailTicks = header.division;
				uint32_t totalSamples = ticksToSamplesWithTempoMap(maxEndTick + tailTicks, tempoEvents, header.division, sampleRate);

				/* Limit maximum duration to 30 minutes to prevent memory issues. */
				constexpr uint32_t MaxDurationSeconds = 30 * 60;
				const uint32_t maxSamples = MaxDurationSeconds * sampleRate;

				if ( totalSamples > maxSamples )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] renderWithSoundfont(), MIDI duration exceeds " << MaxDurationSeconds << "s limit, clamping.\n";
					totalSamples = maxSamples;
				}

				/* Initialize the wave as stereo. */
				if ( !wave.initialize(totalSamples, Channels::Stereo, m_frequency) )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] renderWithSoundfont(), failed to initialize wave !\n";

					return false;
				}

				/* Use a float accumulator buffer for rendering to preserve peaks before normalization. */
				std::vector< float > floatAccumulator(totalSamples * 2, 0.0F);

				/* Build a unified timeline of all events (notes, controls, and tempo changes). */
				struct TimelineEvent
				{
					uint32_t tick;
					enum class Type : uint8_t { NoteOn, NoteOff, Control, TempoChange } type;
					uint8_t channel;
					uint8_t data1;  /* Note number or control type. */
					uint8_t data2;  /* Velocity or control value. */
					int32_t data3;  /* Extended data (pitch bend or tempo). */
				};

				std::vector< TimelineEvent > timeline;
				timeline.reserve(notes.size() * 2 + controlEvents.size() + tempoEvents.size());

				/* Add note events. */
				for ( const auto & note : notes )
				{
					timeline.push_back({note.startTick, TimelineEvent::Type::NoteOn, note.channel, note.noteNumber, note.velocity, 0});
					timeline.push_back({note.endTick, TimelineEvent::Type::NoteOff, note.channel, note.noteNumber, 0, 0});
				}

				/* Add control events. */
				for ( const auto & ctrl : controlEvents )
				{
					timeline.push_back({ctrl.tick, TimelineEvent::Type::Control, ctrl.channel, static_cast< uint8_t >(ctrl.type), ctrl.controller, ctrl.value});
				}

				/* Add tempo events. */
				for ( const auto & tempo : tempoEvents )
				{
					timeline.push_back({tempo.tick, TimelineEvent::Type::TempoChange, 0, 0, 0, static_cast< int32_t >(tempo.tempo)});
				}

				/* Sort by tick. */
				std::sort(timeline.begin(), timeline.end(), [](const TimelineEvent & a, const TimelineEvent & b) {
					return a.tick < b.tick;
				});

				/* Simple sequential rendering: process events in order, render samples between them. */
				uint32_t currentSample = 0;
				uint32_t lastEventTick = 0;
				uint32_t currentTempo = tempoEvents.empty() ? 500000 : tempoEvents[0].tempo;

				/* Vibrato state per channel. */
				std::array< uint8_t, 16 > channelModulation{};  /* CC#1 values. */
				std::array< int16_t, 16 > channelBasePitchBend{};  /* Original pitch bend before vibrato. */
				double vibratoPhase = 0.0;  /* Global vibrato LFO phase. */
				constexpr double VibratoRate = 5.5;  /* Hz - typical vibrato speed. */
				constexpr int VibratoDepthMax = 50;  /* Max pitch bend deviation (~50 cents). */

				/* Process each event in the timeline. */
				for ( size_t eventIndex = 0; eventIndex < timeline.size(); ++eventIndex )
				{
					const auto & event = timeline[eventIndex];

					/* Render samples from last event to this event. */
					if ( event.tick > lastEventTick )
					{
						const uint32_t deltaTicks = event.tick - lastEventTick;
						const double deltaSeconds = (static_cast< double >(deltaTicks) * static_cast< double >(currentTempo)) / (static_cast< double >(header.division) * 1000000.0);
						auto samplesToRender = static_cast< uint32_t >(deltaSeconds * static_cast< double >(sampleRate));

						/* Check if any channel has vibrato active. */
						bool hasVibrato = false;
						for ( uint8_t ch = 0; ch < 16; ++ch )
						{
							if ( channelModulation[ch] > 0 )
							{
								hasVibrato = true;
								break;
							}
						}

						/* Render in chunks for vibrato, or larger chunks otherwise for performance. */
						constexpr uint32_t VibratoChunkSize = 64;
						constexpr uint32_t NormalChunkSize = 4096;  /* Larger chunks when no vibrato. */

						while ( samplesToRender > 0 && currentSample < totalSamples )
						{
							const uint32_t maxChunk = hasVibrato ? VibratoChunkSize : NormalChunkSize;
							const uint32_t chunkSize = std::min(samplesToRender, maxChunk);
							const uint32_t actualSamples = std::min(chunkSize, totalSamples - currentSample);

							/* Apply vibrato LFO to pitch wheel for channels with modulation. */
							if ( hasVibrato )
							{
								const double vibratoValue = std::sin(vibratoPhase * 2.0 * 3.14159265358979323846);

								for ( uint8_t ch = 0; ch < 16; ++ch )
								{
									if ( channelModulation[ch] > 0 )
									{
										/* Scale vibrato depth by modulation amount. */
										const int vibratoOffset = static_cast< int >(vibratoValue * VibratoDepthMax * channelModulation[ch] / 127);
										const int newPitchBend = 8192 + channelBasePitchBend[ch] + vibratoOffset;
										tsf_channel_set_pitchwheel(m_soundfont, ch, std::clamp(newPitchBend, 0, 16383));
									}
								}

								/* Advance vibrato phase. */
								vibratoPhase += VibratoRate * static_cast< double >(actualSamples) / static_cast< double >(sampleRate);
							}

							/* Render directly into the accumulator to avoid extra copy. */
							const size_t outputOffset = static_cast< size_t >(currentSample) * 2;

							if ( outputOffset + actualSamples * 2 <= floatAccumulator.size() )
							{
								tsf_render_float(m_soundfont, floatAccumulator.data() + outputOffset, static_cast< int >(actualSamples), 0);
							}

							currentSample += actualSamples;
							samplesToRender -= actualSamples;
						}

						lastEventTick = event.tick;
					}

					/* Process the event. */
					switch ( event.type )
					{
						case TimelineEvent::Type::NoteOn:
							tsf_channel_note_on(m_soundfont, event.channel, event.data1, static_cast< float >(event.data2) / 127.0F);
							break;

						case TimelineEvent::Type::NoteOff:
							tsf_channel_note_off(m_soundfont, event.channel, event.data1);
							break;

						case TimelineEvent::Type::TempoChange:
							currentTempo = static_cast< uint32_t >(event.data3);
							break;

						case TimelineEvent::Type::Control:
						{
							const auto ctrlType = static_cast< ControlEvent::Type >(event.data1);

							switch ( ctrlType )
							{
								case ControlEvent::Type::PitchBend:
									channelBasePitchBend[event.channel] = static_cast< int16_t >(event.data3);
									tsf_channel_set_pitchwheel(m_soundfont, event.channel, static_cast< int >(event.data3) + 8192);
									break;

								case ControlEvent::Type::Modulation:
									channelModulation[event.channel] = static_cast< uint8_t >(event.data3);
									break;

								case ControlEvent::Type::Volume:
									tsf_channel_set_volume(m_soundfont, event.channel, static_cast< float >(event.data3) / 127.0F);
									break;

								case ControlEvent::Type::Expression:
									tsf_channel_midi_control(m_soundfont, event.channel, 11, static_cast< int >(event.data3));
									break;

								case ControlEvent::Type::Sustain:
									tsf_channel_midi_control(m_soundfont, event.channel, 64, event.data3 != 0 ? 127 : 0);
									break;

								case ControlEvent::Type::Pan:
									tsf_channel_set_pan(m_soundfont, event.channel, static_cast< float >(event.data3) / 127.0F);
									break;

								case ControlEvent::Type::RawMidiCC:
									/* Pass directly to TSF for native handling (Bank Select, RPN, LSB, etc.). */
									tsf_channel_midi_control(m_soundfont, event.channel, event.data2, static_cast< int >(event.data3));
									break;

								case ControlEvent::Type::ChannelPressure:
									/* Channel aftertouch - simulate via expression modulation.
									 * Scale pressure to add expressiveness without overwhelming the base expression. */
									tsf_channel_midi_control(m_soundfont, event.channel, 11, 64 + static_cast< int >(event.data3) / 2);
									break;

								case ControlEvent::Type::PolyKeyPressure:
									/* Per-note aftertouch - TSF doesn't support modifying active voice velocity.
									 * Could be implemented with custom voice tracking, but skipped for now. */
									break;

								case ControlEvent::Type::ProgramChange:
									/* Dynamic instrument change - use bank 128 for drums (channel 9). */
									if ( event.channel == 9 )
									{
										tsf_channel_set_bank_preset(m_soundfont, event.channel, 128, static_cast< int >(event.data3));
									}
									else
									{
										tsf_channel_set_bank_preset(m_soundfont, event.channel, 0, static_cast< int >(event.data3));
									}
									break;

								default:
									break;
							}

							break;
						}
					}
				}

				/* Render any remaining samples after all events. */
				if ( currentSample < totalSamples )
				{
					const uint32_t remainingSamples = totalSamples - currentSample;
					const size_t outputOffset = static_cast< size_t >(currentSample) * 2;

					if ( outputOffset + remainingSamples * 2 <= floatAccumulator.size() )
					{
						tsf_render_float(m_soundfont, floatAccumulator.data() + outputOffset, static_cast< int >(remainingSamples), 0);
					}
				}

				/* Find peak amplitude for normalization. */
				float peakAmplitude = 0.0F;

				for ( const float sample : floatAccumulator )
				{
					peakAmplitude = std::max(peakAmplitude, std::abs(sample));
				}

				/* Calculate normalization factor (with headroom to avoid clipping). */
				constexpr float TargetPeak = 0.95F;  /* Leave 5% headroom. */
				const float normalizationFactor = (std::isfinite(peakAmplitude) && peakAmplitude > 0.0F) ? (TargetPeak / peakAmplitude) : 1.0F;

				/* Convert to output format with normalization. */
				auto & waveData = wave.data();

				for ( size_t i = 0; i < floatAccumulator.size() && i < waveData.size(); ++i )
				{
					const float rawSample = floatAccumulator[i] * normalizationFactor;
					const float normalizedSample = std::isfinite(rawSample) ? rawSample : 0.0F;  /* Guard against NaN/Inf from TSF. */

					if constexpr ( std::is_floating_point_v< precision_t > )
					{
						waveData[i] = static_cast< precision_t >(normalizedSample);
					}
					else
					{
						const auto maxVal = static_cast< float >(std::numeric_limits< precision_t >::max());
						waveData[i] = static_cast< precision_t >(std::clamp(normalizedSample, -1.0F, 1.0F) * maxVal);
					}
				}

				return true;
			}

			/**
			 * @brief Converts MIDI ticks to sample position using tempo map.
			 * @param tick The tick position to convert.
			 * @param tempoEvents The sorted tempo events.
			 * @param division Ticks per quarter note.
			 * @param sampleRate The sample rate in Hz.
			 * @return uint32_t The sample position.
			 */
			[[nodiscard]]
			static uint32_t
			ticksToSamplesWithTempoMap (uint32_t tick, const std::vector< TempoEvent > & tempoEvents, uint16_t division, uint32_t sampleRate) noexcept
			{
				double totalSeconds = 0.0;
				uint32_t lastTick = 0;
				uint32_t currentTempo = 500000; /* Default 120 BPM. */

				for ( const auto & event : tempoEvents )
				{
					if ( event.tick >= tick )
					{
						break;
					}

					/* Add time from lastTick to this tempo event. */
					const uint32_t deltaTicks = event.tick - lastTick;
					totalSeconds += (static_cast< double >(deltaTicks) * static_cast< double >(currentTempo)) / (static_cast< double >(division) * 1000000.0);

					lastTick = event.tick;
					currentTempo = event.tempo;
				}

				/* Add remaining time from last tempo event to target tick. */
				const uint32_t remainingTicks = tick - lastTick;
				totalSeconds += (static_cast< double >(remainingTicks) * static_cast< double >(currentTempo)) / (static_cast< double >(division) * 1000000.0);

				return static_cast< uint32_t >(totalSeconds * static_cast< double >(sampleRate));
			}

			/**
			 * @brief Gets the tempo at a specific tick position.
			 * @param tick The tick position.
			 * @param tempoEvents The sorted tempo events.
			 * @return uint32_t The tempo in microseconds per quarter note.
			 */
			[[nodiscard]]
			static uint32_t
			getTempoAtTick (uint32_t tick, const std::vector< TempoEvent > & tempoEvents) noexcept
			{
				uint32_t tempo = 500000; /* Default 120 BPM. */

				for ( const auto & event : tempoEvents )
				{
					if ( event.tick > tick )
					{
						break;
					}

					tempo = event.tempo;
				}

				return tempo;
			}

			/**
			 * @brief Renders parsed MIDI notes to a stereo Wave with full MIDI controller support.
			 * @param wave The destination wave (will be stereo).
			 * @param notes The parsed notes.
			 * @param controlEvents The control change events (pitch bend, modulation, etc.).
			 * @param tempoEvents The tempo change events (sorted by tick).
			 * @param header The MIDI header for timing info.
			 * @param channelStates The channel states containing pan and program values.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			renderToWave (Wave< precision_t > & wave, const std::vector< MIDINote > & notes, const std::vector< ControlEvent > & controlEvents, const std::vector< TempoEvent > & tempoEvents, const MIDIHeader & header, const std::array< ChannelState, 16 > & channelStates) noexcept
			{
				/* If a SoundFont is available, use sample-based rendering. */
				if ( m_soundfont != nullptr )
				{
					return this->renderWithSoundfont(wave, notes, controlEvents, tempoEvents, header, channelStates);
				}

				/* Otherwise, fall back to additive synthesis. */
				const auto sampleRate = static_cast< uint32_t >(m_frequency);
				const auto sampleRateF = static_cast< float >(sampleRate);

				/* Find the last note end to determine total duration. */
				uint32_t maxEndTick = 0;

				for ( const auto & note : notes )
				{
					maxEndTick = std::max(maxEndTick, note.endTick);
				}

				/* Add a small tail for release. */
				const uint32_t tailTicks = header.division / 2;
				const uint32_t totalSamples = ticksToSamplesWithTempoMap(maxEndTick + tailTicks, tempoEvents, header.division, sampleRate);

				/* Initialize the wave as stereo. */
				if ( !wave.initialize(totalSamples, Channels::Stereo, m_frequency) )
				{
					std::cerr << "[WaveFactory::FileFormatMIDI] renderToWave(), failed to initialize wave !\n";

					return false;
				}

				/* Clear the wave data. */
				std::fill(wave.data().begin(), wave.data().end(), precision_t{0});

				/* Build channel event index for fast lookup. */
				const auto channelEventIndex = buildChannelEventIndex(controlEvents);

				/* Get initial tempo for tick-to-sample conversion factor. */
				const uint32_t initialTempo = getTempoAtTick(0, tempoEvents);
				const float tickToSampleFactor = static_cast< float >(initialTempo) / (static_cast< float >(header.division) * 1000000.0F) * sampleRateF;
				const float sampleToTickFactor = (tickToSampleFactor > 0.0F) ? (1.0F / tickToSampleFactor) : 0.0F;  /* Pre-computed inverse for faster multiplication. */

				/* Random generator for noise. */
				std::random_device randomDevice;
				std::mt19937 generator{randomDevice()};
				std::uniform_real_distribution< float > noiseDist(-1.0F, 1.0F);

				auto & waveData = wave.data();

				/* Control update interval in samples (update controls every ~5ms). */
				constexpr uint32_t ControlUpdateInterval = 256;

				/* Sort notes by start time for portamento detection.
				 * We need to track the previous note on each channel. */
				std::vector< MIDINote > sortedNotes = notes;
				std::sort(sortedNotes.begin(), sortedNotes.end(), [](const MIDINote & a, const MIDINote & b) {
					return a.startTick < b.startTick;
				});

				/* Track the last note frequency per channel for portamento. */
				std::array< float, 16 > lastNoteFrequency{};
				std::array< uint32_t, 16 > lastNoteEndTick{};
				lastNoteFrequency.fill(0.0F);
				lastNoteEndTick.fill(0);

				/* Render each note with pitch bend, modulation, expression, volume, and portamento. */
				for ( const auto & note : sortedNotes )
				{
					const uint32_t startSample = ticksToSamplesWithTempoMap(note.startTick, tempoEvents, header.division, sampleRate);
					const uint32_t endSample = ticksToSamplesWithTempoMap(note.endTick, tempoEvents, header.division, sampleRate);

					if ( endSample <= startSample || startSample >= totalSamples ) [[unlikely]]
					{
						continue;
					}

					const uint32_t noteSamples = std::min(endSample, totalSamples) - startSample;
					const float noteDuration = static_cast< float >(noteSamples) / sampleRateF;
					const float baseFrequency = Synth::noteToFrequency(note.noteNumber);
					const float baseAmplitude = static_cast< float >(note.velocity) / 127.0F * 0.25F;
					const float pitchBendRange = channelStates[note.channel].pitchBendRange;

					/* Determine portamento parameters.
					 * Portamento glides from the previous note's frequency to this note's frequency. */
					const float prevFrequency = lastNoteFrequency[note.channel];
					const bool hasPortamento = (prevFrequency > 0.0F) &&
						channelStates[note.channel].portamentoOn &&
						(channelStates[note.channel].portamentoTime > 0) &&
						(note.channel != 9); /* No portamento on percussion. */

					/* Calculate portamento duration in samples. */
					const float portamentoDuration = hasPortamento ? Synth::portamentoTimeToSeconds(channelStates[note.channel].portamentoTime) : 0.0F;
					const auto portamentoSamples = static_cast< uint32_t >(portamentoDuration * sampleRateF);

					/* Update last note tracking for this channel. */
					lastNoteFrequency[note.channel] = baseFrequency;
					lastNoteEndTick[note.channel] = note.endTick;

					/* Get instrument family and ADSR. */
					const auto family = Synth::getInstrumentFamily(channelStates[note.channel].program);
					float attack, decay, sustain, release;

					if ( note.channel == 9 )
					{
						/* Percussion: very short envelope. */
						attack = 0.001F;
						decay = std::min(0.05F, noteDuration * 0.3F);
						sustain = 0.0F;
						release = std::min(0.05F, noteDuration * 0.2F);
					}
					else
					{
						getADSRForFamily(family, noteDuration, attack, decay, sustain, release);
					}

					/* Calculate stereo pan gains. */
					const float panValue = static_cast< float >(channelStates[note.channel].pan) / 127.0F;
					const float panAngle = panValue * 1.5707963F;
					const float leftGain = std::cos(panAngle);
					const float rightGain = std::sin(panAngle);

					/* Initialize control cache for this note's channel. */
					ChannelControlCache cache;
					size_t searchIndex = 0;
					const auto & channelEvents = channelEventIndex[note.channel];

					/* Find the control state at note start by processing events up to startTick. */
					updateControlCache(controlEvents, channelEvents, cache, searchIndex, note.startTick);

					/* Cache current control-derived values. */
					float bendMultiplier = std::exp2((static_cast< float >(cache.pitchBend) / 8192.0F) * pitchBendRange / 12.0F);
					float modulationDepth = static_cast< float >(cache.modulation) / 127.0F;
					float expressionFactor = static_cast< float >(cache.expression) / 127.0F;
					float volumeFactor = static_cast< float >(cache.volume) / 127.0F;

					/* Filter parameters: 2-pole resonant low-pass filter (Moog-style ladder approximation). */
					float filterCutoff = Synth::filterCutoffToCoefficient(cache.filterCutoff, sampleRateF);
					float filterResonance = Synth::filterResonanceToFeedback(cache.filterResonance);
					float filterBuf0 = 0.0F; /* Filter state buffer 0. */
					float filterBuf1 = 0.0F; /* Filter state buffer 1. */

					/* Tremolo depth (0.0 to 1.0). */
					float tremoloDepth = static_cast< float >(cache.tremoloDepth) / 127.0F;

					/* Track phase accumulation for continuous waveform. */
					float phase = 0.0F;

					/* Process each sample of the note. */
					for ( uint32_t localSample = 0; localSample < noteSamples; ++localSample )
					{
						const uint32_t globalSample = startSample + localSample;

						/* Update control values periodically (not every sample). */
						if ( localSample % ControlUpdateInterval == 0 )
						{
							/* Calculate current tick (multiplication faster than division). */
							const auto currentTick = static_cast< uint32_t >(static_cast< float >(globalSample) * sampleToTickFactor);

							/* Update cache with any new events. */
							updateControlCache(controlEvents, channelEvents, cache, searchIndex, currentTick);

							/* Recalculate derived values. */
							bendMultiplier = std::exp2((static_cast< float >(cache.pitchBend) / 8192.0F) * pitchBendRange / 12.0F);
							modulationDepth = static_cast< float >(cache.modulation) / 127.0F;
							expressionFactor = static_cast< float >(cache.expression) / 127.0F;
							volumeFactor = static_cast< float >(cache.volume) / 127.0F;

							/* Update filter parameters. */
							filterCutoff = Synth::filterCutoffToCoefficient(cache.filterCutoff, sampleRateF);
							filterResonance = Synth::filterResonanceToFeedback(cache.filterResonance);

							/* Update tremolo depth. */
							tremoloDepth = static_cast< float >(cache.tremoloDepth) / 127.0F;
						}

						/* Calculate vibrato from modulation (CC#1). */
						constexpr float VibratoRate = 5.5F;
						constexpr float MaxVibratoDepth = 0.02F;
						const float vibratoTime = static_cast< float >(localSample) / sampleRateF;
						const float vibratoMultiplier = 1.0F + MaxVibratoDepth * modulationDepth * std::sin(2.0F * 3.14159265F * VibratoRate * vibratoTime);

						/* Calculate tremolo (amplitude modulation) from CC#92.
						 * Tremolo rate is typically 4-7 Hz, we use 5.0 Hz.
						 * The effect oscillates between (1.0 - depth) and 1.0. */
						constexpr float TremoloRate = 5.0F;
						const float tremoloMultiplier = 1.0F - tremoloDepth * 0.5F * (1.0F - std::sin(2.0F * 3.14159265F * TremoloRate * vibratoTime));

						/* Calculate portamento glide (logarithmic interpolation between frequencies).
						 * During the portamento period, glide from prevFrequency to baseFrequency. */
						float portamentoFrequency = baseFrequency;

						if ( hasPortamento && localSample < portamentoSamples && portamentoSamples > 0 ) [[unlikely]]
						{
							/* Logarithmic interpolation for musical pitch glide. */
							const float glideProgress = static_cast< float >(localSample) / static_cast< float >(portamentoSamples);
							const float frequencyRatio = baseFrequency / prevFrequency;

							/* Guard against invalid ratio (prevents NaN from log2). */
							if ( frequencyRatio > 0.0F ) [[likely]]
							{
								portamentoFrequency = prevFrequency * std::exp2(glideProgress * std::log2(frequencyRatio));
							}
						}

						/* Calculate final frequency with all modifiers. */
						const float frequency = portamentoFrequency * bendMultiplier * vibratoMultiplier;

						/* Calculate amplitude with tremolo. */
						const float envelope = Synth::calculateEnvelopeSample(localSample, sampleRate, noteSamples, attack, decay, sustain, release);
						const float amplitude = baseAmplitude * expressionFactor * volumeFactor * envelope * tremoloMultiplier;

						/* Generate sample based on instrument type. */
						float sample;

						if ( note.channel == 9 )
						{
							/* Percussion: noise burst. */
							sample = amplitude * noiseDist(generator);
						}
						else
						{
							/* Generate waveform sample. */
							sample = amplitude * Synth::generateWaveformSample(family, phase);

							/* Accumulate phase for next sample. */
							phase += frequency / sampleRateF;

							/* Wrap phase to [0, 1) range. Using fmod for safety with high frequencies. */
							if ( phase >= 1.0F ) [[unlikely]]
							{
								phase = std::fmod(phase, 1.0F);
							}

							/* Apply resonant low-pass filter (2-pole Moog-style ladder approximation).
							 * The filter only applies to melodic instruments, not percussion. */
							if ( filterCutoff < 0.99F ) [[likely]]
							{
								/* Calculate feedback based on resonance and cutoff.
								 * Higher resonance at lower cutoffs gives the classic "squelchy" sound. */
								const float feedback = filterResonance + filterResonance / (1.0F - filterCutoff * 0.5F);

								/* Two-pole filter with resonance feedback. */
								filterBuf0 += filterCutoff * (sample - filterBuf0 + feedback * (filterBuf0 - filterBuf1));
								filterBuf1 += filterCutoff * (filterBuf0 - filterBuf1);

								/* Use the second buffer as output (2-pole rolloff = -12dB/octave). */
								sample = filterBuf1;
							}
						}

						/* Mix into stereo output. */
						const size_t stereoIndex = static_cast< size_t >(globalSample) * 2;

						if ( stereoIndex + 1 < waveData.size() )
						{
							if constexpr ( std::is_floating_point_v< precision_t > )
							{
								waveData[stereoIndex] += static_cast< precision_t >(sample * leftGain);
								waveData[stereoIndex + 1] += static_cast< precision_t >(sample * rightGain);
							}
							else
							{
								const auto maxVal = static_cast< float >(std::numeric_limits< precision_t >::max());
								const auto leftSample = static_cast< int32_t >(sample * leftGain * maxVal);
								const auto rightSample = static_cast< int32_t >(sample * rightGain * maxVal);

								const int32_t leftSum = static_cast< int32_t >(waveData[stereoIndex]) + leftSample;
								const int32_t rightSum = static_cast< int32_t >(waveData[stereoIndex + 1]) + rightSample;

								waveData[stereoIndex] = static_cast< precision_t >(std::clamp(leftSum, static_cast< int32_t >(std::numeric_limits< precision_t >::lowest()), static_cast< int32_t >(std::numeric_limits< precision_t >::max())));
								waveData[stereoIndex + 1] = static_cast< precision_t >(std::clamp(rightSum, static_cast< int32_t >(std::numeric_limits< precision_t >::lowest()), static_cast< int32_t >(std::numeric_limits< precision_t >::max())));
							}
						}
					}
				}

				/* Normalize the stereo output. */
				Processor processor{wave};
				processor.normalize();
				processor.toWave(wave);

				return true;
			}

			Frequency m_frequency{Frequency::PCM48000Hz};
			tsf * m_soundfont{nullptr};
	};
}
