/*
 * Copyright 2011-2016 "Silver Squirrel Software Handelsbolag"
 * Copyright 2023-2025 "John Högberg"
 *
 * This file is part of tibiarc.
 *
 * tibiarc is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * tibiarc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with tibiarc. If not, see <https://www.gnu.org/licenses/>.
 */

#include "serializer.hpp"
#include "cli.hpp"

#include <unordered_set>
#include <climits>

using namespace trc;

int main(int argc, char **argv) {
    /* Set up some sane defaults. */
    Serializer::Settings settings = {
            .InputFormat = Recordings::Format::Unknown,
            .InputRecovery = Recordings::Recovery::None,

            .StartTime = std::chrono::milliseconds::zero(),
            .EndTime = std::chrono::milliseconds::max(),

            .DryRun = false};

    auto paths = CLI::Process(
            argc,
            argv,
            "tibiarc-miner -- a program for converting Tibia packet captures "
            "to JSON",
            "tibiarc-miner 0.4",
            {"data_folder", "input_path"},
            {
                    {"end-time",
                     {"when to stop encoding, in milliseconds relative to "
                      "start",
                      {"end_ms"},
                      [&](const CLI::Range &args) {
                          uint32_t time;
                          if (sscanf(args[0].c_str(), "%i", &time) != 1 ||
                              time < 0) {
                              throw "end-time must be a time in milliseconds";
                          }

                          settings.EndTime = std::chrono::milliseconds(time);
                      }}},
                    {"start-time",
                     {"when to start encoding, in milliseconds relative "
                      "to start",
                      {"start_ms"},
                      [&](const CLI::Range &args) {
                          uint32_t time;
                          if (sscanf(args[0].c_str(), "%i", &time) != 1 ||
                              time < 0) {
                              throw "start-time must be a time in milliseconds";
                          }

                          settings.StartTime = std::chrono::milliseconds(time);
                      }}},

                    {"input-format",
                     {"the format of the recording, 'cam', 'rec', 'tibiacast', "
                      "'tmv1', 'tmv2', 'trp', or 'yatc'.",
                      {"format"},
                      [&](const CLI::Range &args) {
                          const auto &format = args[0];

                          if (format == "cam") {
                              settings.InputFormat = Recordings::Format::Cam;
                          } else if (format == "rec") {
                              settings.InputFormat = Recordings::Format::Rec;
                          } else if (format == "tibiacast") {
                              settings.InputFormat =
                                      Recordings::Format::Tibiacast;
                          } else if (format == "tmv1") {
                              settings.InputFormat =
                                      Recordings::Format::TibiaMovie1;
                          } else if (format == "tmv2") {
                              settings.InputFormat =
                                      Recordings::Format::TibiaMovie2;
                          } else if (format == "trp") {
                              settings.InputFormat =
                                      Recordings::Format::TibiaReplay;
                          } else if (format == "ttm") {
                              settings.InputFormat =
                                      Recordings::Format::TibiaTimeMachine;
                          } else if (format == "yatc") {
                              settings.InputFormat = Recordings::Format::YATC;
                          } else {
                              throw "input-format must be 'cam', 'rec', "
                                    "'tibiacast', 'tmv1', 'tmv2', 'trp', "
                                    "'ttm', "
                                    "or 'yatc'";
                          }
                      }}},
                    {"input-version",
                     {"the Tibia version of the recording, in case the "
                      "automatic detection doesn't work",
                      {"tibia_version"},
                      [&](const CLI::Range &args) {
                          if (sscanf(args[0].c_str(),
                                     "%u.%u.%u",
                                     &settings.DesiredTibiaVersion.Major,
                                     &settings.DesiredTibiaVersion.Minor,
                                     &settings.DesiredTibiaVersion.Preview) <
                              2) {
                              throw "input-version must be in the format "
                                    "'X.Y', e.g. '8.55'";
                          }
                      }}},

                    {"skip-creature-presence",
                     {"skips creature presence events",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          settings.SkippedEvents.insert(
                                  Events::Type::CreatureSeen);
                          settings.SkippedEvents.insert(
                                  Events::Type::CreatureRemoved);
                      }}},
                    {"skip-creature-updates",
                     {"skips creature update events (e.g. movement, health)",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::CreatureGuildMembersUpdated,
                                Events::Type::CreatureHeadingUpdated,
                                Events::Type::CreatureHealthUpdated,
                                Events::Type::CreatureImpassableUpdated,
                                Events::Type::CreatureLightUpdated,
                                Events::Type::CreatureMoved,
                                Events::Type::CreatureNPCCategoryUpdated,
                                Events::Type::CreatureOutfitUpdated,
                                Events::Type::CreaturePvPHelpersUpdated,
                                Events::Type::CreatureShieldUpdated,
                                Events::Type::CreatureSkullUpdated,
                                Events::Type::CreatureSpeedUpdated}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"skip-effects",
                     {"skips effect events (e.g. missiles, poofs)",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::GraphicalEffectPopped,
                                Events::Type::NumberEffectPopped,
                                Events::Type::MissileFired}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"skip-inventory",
                     {"skips inventory events (e.g. containers)",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::ContainerAddedItem,
                                Events::Type::ContainerClosed,
                                Events::Type::ContainerOpened,
                                Events::Type::ContainerRemovedItem,
                                Events::Type::ContainerTransformedItem,
                                Events::Type::PlayerInventoryUpdated}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"skip-messages",
                     {"skips message events",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::ChannelClosed,
                                Events::Type::ChannelListUpdated,
                                Events::Type::ChannelOpened,
                                Events::Type::CreatureSpoke,
                                Events::Type::CreatureSpokeInChannel,
                                Events::Type::CreatureSpokeOnMap,
                                Events::Type::StatusMessageReceived,
                                Events::Type::StatusMessageReceivedInChannel}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"skip-player-updates",
                     {"skips player update events (e.g. movement, skills)",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::PlayerBlessingsUpdated,
                                Events::Type::PlayerDataBasicUpdated,
                                Events::Type::PlayerDataUpdated,
                                Events::Type::PlayerHotkeyPresetUpdated,
                                Events::Type::PlayerMoved,
                                Events::Type::PlayerSkillsUpdated}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"skip-terrain",
                     {"skips terrain events",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::TileUpdated,
                                Events::Type::TileObjectAdded,
                                Events::Type::TileObjectRemoved,
                                Events::Type::TileObjectTransformed}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"terrain-only",
                     {"skips everything but terrain events (suitable for map "
                      "tracking)",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          for (const auto event :
                               {Events::Type::WorldInitialized,
                                Events::Type::AmbientLightChanged,
                                Events::Type::CreatureMoved,
                                Events::Type::CreatureRemoved,
                                Events::Type::CreatureSeen,
                                Events::Type::CreatureHealthUpdated,
                                Events::Type::CreatureHeadingUpdated,
                                Events::Type::CreatureLightUpdated,
                                Events::Type::CreatureOutfitUpdated,
                                Events::Type::CreatureSpeedUpdated,
                                Events::Type::CreatureSkullUpdated,
                                Events::Type::CreatureShieldUpdated,
                                Events::Type::CreatureImpassableUpdated,
                                Events::Type::CreaturePvPHelpersUpdated,
                                Events::Type::CreatureGuildMembersUpdated,
                                Events::Type::CreatureTypeUpdated,
                                Events::Type::CreatureNPCCategoryUpdated,
                                Events::Type::PlayerMoved,
                                Events::Type::PlayerInventoryUpdated,
                                Events::Type::PlayerBlessingsUpdated,
                                Events::Type::PlayerDied,
                                Events::Type::PlayerHotkeyPresetUpdated,
                                Events::Type::PlayerDataBasicUpdated,
                                Events::Type::PlayerDataUpdated,
                                Events::Type::PlayerSkillsUpdated,
                                Events::Type::PlayerIconsUpdated,
                                Events::Type::PlayerTacticsUpdated,
                                Events::Type::PvPSituationsChanged,
                                Events::Type::CreatureSpoke,
                                Events::Type::CreatureSpokeOnMap,
                                Events::Type::CreatureSpokeInChannel,
                                Events::Type::ChannelListUpdated,
                                Events::Type::ChannelOpened,
                                Events::Type::ChannelClosed,
                                Events::Type::PrivateConversationOpened,
                                Events::Type::ContainerOpened,
                                Events::Type::ContainerClosed,
                                Events::Type::ContainerAddedItem,
                                Events::Type::ContainerTransformedItem,
                                Events::Type::ContainerRemovedItem,
                                Events::Type::NumberEffectPopped,
                                Events::Type::GraphicalEffectPopped,
                                Events::Type::MissileFired,
                                Events::Type::StatusMessageReceived,
                                Events::Type::StatusMessageReceivedInChannel}) {
                              settings.SkippedEvents.insert(event);
                          }
                      }}},
                    {"dry-run",
                     {"suppress output while still generating it. This is only"
                      "intended for testing",
                      {},
                      [&]([[maybe_unused]] const CLI::Range &args) {
                          settings.DryRun = true;
                      }}},
            });

    try {
        Serializer::Serialize(settings, paths[0], paths[1], std::cout);
    } catch (const ErrorBase &error) {
        std::cerr << "Unrecoverable error (" << error.Description() << ")"
                  << std::endl;
        return 1;
    }

    return 0;
}
