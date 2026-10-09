#pragma once

namespace dryguard {
namespace storage_keys {

static const char kCurrentNamespace[] = "dryguard_v3";
static const char kStateKey[] = "state";

// Read-only migration aliases for the previous checkpoint namespace.
static const char kPreviousV3Namespace[] = "jemuran_v3";

// Read-only migration aliases for earlier Preferences field stores.
static const char kLegacyV2Namespace[] = "jemuran_v2";
static const char kLegacyV1Namespace[] = "jemuran";
static const char kLegacyPositionKey[] = "posisi";
static const char kLegacyTargetKey[] = "target";
static const char kLegacyAutomaticModeKey[] = "modeAuto";
static const char kLegacyManualModeKey[] = "modeManual";
static const char kLegacyResumeFlagKey[] = "res_flag";
static const char kLegacyResumeTargetKey[] = "res_tgt";
static const char kLegacyResumePositionKey[] = "res_pos";

}  // namespace storage_keys
}  // namespace dryguard
