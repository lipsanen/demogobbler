#include "benchmark/benchmark.h"

extern "C" {
  #include "demogobbler_hashtable.h"
  #define XXH_INLINE_ALL
  #include "xxhash.h"
}
#include <map>
#include <unordered_map>
#include <stdlib.h>
#include <string.h>

static const char *TEST_STRINGS[] = {
    "DT_AR2Explosion",
    "DT_AI_BaseNPC",
    "DT_AlyxEmpEffect",
    "DT_BaseAnimating",
    "DT_BaseAnimatingOverlay",
    "DT_BaseCombatCharacter",
    "DT_BaseCombatWeapon",
    "DT_BaseDoor",
    "DT_BaseEntity",
    "DT_BaseFlex",
    "DT_BaseGrenade",
    "DT_BaseHelicopter",
    "DT_BaseHLBludgeonWeapon",
    "DT_BaseHLCombatWeapon",
    "DT_BaseParticleEntity",
    "DT_BasePlayer",
    "DT_BasePortalCombatWeapon",
    "DT_BasePropDoor",
    "DT_BaseTempEntity",
    "DT_BaseViewModel",
    "DT_Beam",
    "DT_BoneFollower",
    "DT_BreakableProp",
    "DT_BreakableSurface",
    "DT_CitadelEnergyCore",
    "DT_ColorCorrection",
    "DT_ColorCorrectionVolume",
    "DT_Corpse",
    "DT_CrossbowBolt",
    "DT_DinosaurSignal",
    "DT_DynamicLight",
    "DT_DynamicProp",
    "DT_Embers",
    "DT_EntityDissolve",
    "DT_EntityFlame",
    "DT_EntityParticleTrail",
    "DT_Env_Lightrail_Endpoint",
    "DT_DetailController",
    "DT_EnvHeadcrabCanister",
    "DT_EnvParticleScript",
    "DT_EnvPortalPathTrack",
    "DT_EnvProjectedTexture",
    "DT_QuadraticBeam",
    "DT_EnvScreenEffect",
    "DT_EnvScreenOverlay",
    "DT_EnvStarfield",
    "DT_EnvTonemapController",
    "DT_EnvWind",
    "DT_FireSmoke",
    "DT_FireTrail",
    "DT_CFish",
    "DT_Flare",
    "DT_FleshEffectTarget",
    "DT_FogController",
    "DT_Func_Dust",
    "DT_Func_LiquidPortal",
    "DT_Func_LOD",
    "DT_FuncAreaPortalWindow",
    "DT_FuncConveyor",
    "DT_FuncLadder",
    "DT_FuncMonitor",
    "DT_FuncOccluder",
    "DT_FuncReflectiveGlass",
    "DT_FuncRotating",
    "DT_FuncSmokeVolume",
    "DT_FuncTrackTrain",
    "DT_GameRulesProxy",
    "DT_GrenadeHopwire",
    "DT_HalfLife2Proxy",
    "DT_HandleTest",
    "DT_HL2_Player",
    "DT_HLMachineGun",
    "DT_HLSelectFireMachineGun",
    "DT_InfoLadderDismount",
    "DT_InfoLightingRelative",
    "DT_InfoOverlayAccessor",
    "DT_InfoTeleporterCountdown",
    "DT_LightGlow",
    "DT_MaterialModifyControl",
    "DT_MortarShell",
    "DT_NeurotoxinCountdown",
    "DT_NPC_AntlionGuard",
    "DT_Barnacle",
    "DT_NPC_Barney",
    "DT_CombineGunship",
    "DT_NPC_Manhack",
    "DT_NPC_Portal_FloorTurret",
    "DT_NPC_RocketTurret",
    "DT_RollerMine",
    "DT_NPC_Strider",
    "DT_NPC_Vortigaunt",
    "DT_ParticleFire",
    "DT_ParticlePerformanceMonitor",
    "DT_ParticleSystem",
    "DT_PhysBox",
    "DT_PhysBoxMultiplayer",
    "DT_PhysicsProp",
    "DT_PhysicsPropMultiplayer",
    "DT_PhysMagnet",
    "DT_Plasma",
    "DT_PlayerResource",
    "DT_PointCamera",
    "DT_PointCommentaryNode",
    "DT_PropDinosaur",
    "DT_Portal_Player",
    "DT_PortalGameRulesProxy",
    "DT_PortalRagdoll",
    "DT_PoseController",
    "DT_Precipitation",
    "DT_Prop_Portal",
    "DT_PropAirboat",
    "DT_PropCannon",
    "DT_PropCombineBall",
    "DT_PropCrane",
    "DT_PropEnergyBall",
    "DT_PropJeep",
    "DT_CPropJeepEpisodic",
    "DT_PropPortalStatsDisplay",
    "DT_PropVehicleChoreoGeneric",
    "DT_PropVehicleDriveable",
    "DT_PropVehiclePrisonerPod",
    "DT_RagdollManager",
    "DT_Ragdoll",
    "DT_Ragdoll_Attached",
    "DT_RopeKeyframe",
    "DT_RotorWashEmitter",
    "DT_SceneEntity",
    "DT_ScriptIntro",
    "DT_ShadowControl",
    "DT_SlideshowDisplay",
    "DT_SmokeStack",
    "DT_SpotlightEnd",
    "DT_Sprite",
    "DT_SpriteOriented",
    "DT_SpriteTrail",
    "DT_SteamJet",
    "DT_Sun",
    "DT_Team",
    "DT_TEAntlionDust",
    "DT_TEArmorRicochet",
    "DT_BaseBeam",
    "DT_TEBeamEntPoint",
    "DT_TEBeamEnts",
    "DT_TEBeamFollow",
    "DT_TEBeamLaser",
    "DT_TEBeamPoints",
    "DT_TEBeamRing",
    "DT_TEBeamRingPoint",
    "DT_TEBeamSpline",
    "DT_TEBloodSprite",
    "DT_TEBloodStream",
    "DT_TEBreakModel",
    "DT_TEBSPDecal",
    "DT_TEBubbles",
    "DT_TEBubbleTrail",
    "DT_TEClientProjectile",
    "DT_TEConcussiveExplosion",
    "DT_TEDecal",
    "DT_TEDust",
    "DT_TEDynamicLight",
    "DT_TEEffectDispatch",
    "DT_TEEnergySplash",
    "DT_TEExplosion",
    "DT_TEFizz",
    "DT_TEFootprintDecal",
    "DT_TEGaussExplosion",
    "DT_TEGlowSprite",
    "DT_TEImpact",
    "DT_TEKillPlayerAttachments",
    "DT_TELargeFunnel",
    "DT_TEMetalSparks",
    "DT_TEMuzzleFlash",
    "DT_TEParticleSystem",
    "DT_TEPhysicsProp",
    "DT_TEPlayerAnimEvent",
    "DT_TEPlayerDecal",
    "DT_TEProjectedDecal",
    "DT_TEShatterSurface",
    "DT_TEShowLine",
    "DT_Tesla",
    "DT_TESmoke",
    "DT_TESparks",
    "DT_TESprite",
    "DT_TESpriteSpray",
    "DT_ProxyToggle",
    "DT_TestTraceline",
    "DT_TEWorldDecal",
    "DT_VGuiScreen",
    "DT_VortigauntChargeToken",
    "DT_VortigauntEffectDispel",
    "DT_WaterBullet",
    "DT_WaterLODControl",
    "DT_Weapon357",
    "DT_WeaponAlyxGun",
    "DT_WeaponAnnabelle",
    "DT_WeaponAR2",
    "DT_WeaponBugBait",
    "DT_WeaponCitizenPackage",
    "DT_WeaponCitizenSuitcase",
    "DT_WeaponCrossbow",
    "DT_WeaponCrowbar",
    "DT_WeaponCubemap",
    "DT_WeaponCycler",
    "DT_WeaponFrag",
    "DT_WeaponHopwire",
    "DT_WeaponPhysCannon",
    "DT_WeaponPistol",
    "DT_WeaponPortalBase",
    "DT_WeaponPortalgun",
    "DT_WeaponRPG",
    "DT_WeaponShotgun",
    "DT_WeaponSMG1",
    "DT_WeaponStunStick",
    "DT_WORLD",
    "DT_DustTrail",
    "DT_MovieExplosion",
    "DT_NextBot",
    "DT_ParticleSmokeGrenade",
    "DT_RocketTrail",
    "DT_SmokeTrail",
    "DT_SporeExplosion",
    "DT_SporeTrail",
};

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  (size_t)(!(sizeof(a) % sizeof(*(a)))))


const size_t TIMES_SEARCHED = 1;

static void hashmap_custom(benchmark::State &state) {
  const size_t array_size = ARRAYSIZE(TEST_STRINGS);

  for(auto _ : state) {
    auto table = demogobbler_hashtable_create(array_size * 2);

    for(size_t i=0; i < array_size; ++i) {
      hashtable_entry entry;
      entry.str = TEST_STRINGS[i];
      entry.value = i;
      demogobbler_hashtable_insert(&table, entry);
    }

    for(size_t u=0; u < TIMES_SEARCHED; ++u) {
      for(size_t i=0; i < array_size; ++i) {
        auto entry = demogobbler_hashtable_get(&table, TEST_STRINGS[i]);
        if(entry.value != i) {
          abort();
        }
      }
    }
    demogobbler_hashtable_free(&table);
  }
}

struct keyhash {
  std::size_t operator()(const char* str) const {
    return XXH32(str, strlen(str), 0);
  }
};

struct strequal {
  bool operator()(const char* lhs, const char* rhs) const {
    return strcmp(rhs, lhs) == 0;
  }
};

const float LOAD_FACTOR = 0.9f;

static void hashmap_unordered_map(benchmark::State &state) {
  const size_t array_size = ARRAYSIZE(TEST_STRINGS);
  const size_t allocated_size = static_cast<size_t>(array_size / LOAD_FACTOR);

  for(auto _ : state) {
    std::unordered_map<const char*, size_t, keyhash, strequal> map;
    map.reserve(allocated_size);

    for(size_t i=0; i < array_size; ++i) {
      map.insert({TEST_STRINGS[i], i});
    }

    for(size_t u=0; u < TIMES_SEARCHED; ++u) {
      for(size_t i=0; i < array_size; ++i) {
        auto it = map.find(TEST_STRINGS[i]);
        if(it->second != i) {
          abort();
        }
      }
    }
  }
}


struct compare {
  bool operator()(const char* lhs, const char* rhs) const {
    return strcmp(lhs, rhs) > 0;
  }
};

static void hashmap_map(benchmark::State &state) {
  const size_t array_size = ARRAYSIZE(TEST_STRINGS);

  for(auto _ : state) {
    std::map<const char*, size_t, compare> map;

    for(size_t i=0; i < array_size; ++i) {
      map.insert({TEST_STRINGS[i], i});
    }

    for(size_t u=0; u < TIMES_SEARCHED; ++u) {
      for(size_t i=0; i < array_size; ++i) {
        auto it = map.find(TEST_STRINGS[i]);
        if(it->second != i) {
          abort();
        }
      }
    }
  }
}

static size_t nomap_find(const char* str) {
  const size_t array_size = ARRAYSIZE(TEST_STRINGS);
  for(size_t i=0; i < array_size; ++i) {
    if(strcmp(str, TEST_STRINGS[i]) == 0)
      return i;
  }

  abort();
}


static void hashmap_nomap(benchmark::State &state) {
  const size_t array_size = ARRAYSIZE(TEST_STRINGS);
  for(auto _ : state) {
    for(size_t u=0; u < TIMES_SEARCHED; ++u) {
      for(size_t i=0; i < array_size; ++i) {
        auto value = nomap_find(TEST_STRINGS[i]);
        if(value != i) {
          abort();
        }
      }
    }
  }
}

BENCHMARK(hashmap_custom);
BENCHMARK(hashmap_nomap);
BENCHMARK(hashmap_map);
BENCHMARK(hashmap_unordered_map);
