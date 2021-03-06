#include <fstream>

#include <minecraft/core/getSP.h>
#include <minecraft/core/types.h>
#include <minecraft/level/Level.h>
#include <minecraft/actor/Player.h>
#include <minecraft/actor/ActorDamageSource.h>
#include <Loader.h>
//#include <MC.h>
#include <cmdhelper.h>
#include <minecraft/json.h>

#include "base.h"
#include "lang.h"
#include "../money/money.h"
#include "main.command.h"

const char meta[] __attribute__((used, section("meta"))) =
    "name:kbonus\n"
    "version:20200121\n"
    "author:sysca11\n"
    "depend:base@20200121,command@20200121,money@20200121\n";

extern "C" {
BDL_EXPORT void mod_init(std::list<string> &modlist);
}
extern void load_helper(std::list<string> &modlist);
unordered_map<int, std::pair<int, int>> bonus_mp;

void load(CommandOutput *out) {
  std::ifstream ifs{"config/killbonus.json"};
  Json::Value value;
  Json::Reader reader;
  if (!reader.parse(ifs, value)) {
    auto msg = reader.getFormattedErrorMessages();
    do_log("%s", msg.c_str());
    if (out)
      out->error(msg);
    else
      exit(1);
  }
  for (auto &i : value) {
    auto bMin     = i["MinMoney"].asInt(0);
    auto bMax     = i["MaxMoney"].asInt(0);
    auto eid      = i["eid"].asInt(0);
    bonus_mp[eid] = {bMin, bMax};
  }
  if (out) out->success(L_KBONUS_RELOADED);//L_KBONUS_RELOADED
}
static int dbg_die;
static void toggle_dbg(CommandOutput *out) {
  dbg_die = !dbg_die;
  if (out) out->success(std::string(L_KBONUS_DEBUG_MODE) + (dbg_die ? L_KBONUS_DEBUG_ON : L_KBONUS_DEBUG_OFF));//L_KBONUS_DEBUG_MODE L_KBONUS_DEBUG_ON L_KBONUS_DEBUG_OFF
}
static void handle_die(Mob &a, ActorDamageSource const &b) {
  if (b.isChildEntitySource() || b.isEntitySource()) {
    auto ent         = getSrvLevel()->fetchEntity(b.getEntityUniqueID(), false);
    ServerPlayer *sp = getSP(ent);
    if (sp) {
      auto vid = a.getEntityTypeId();
      if (dbg_die) {
        char buf[128];
        snprintf(buf,128,L_KBONUS_KILL_TEXT_DEBUG,vid);//L_KBONUS_KILL_TEXT
        sendText(sp,string(buf));
        // sendText(sp, L_KBONUS_KILL_TEXT + std::to_string(vid));
      }//L_KBONUS_KILL_TEXT
      auto it = bonus_mp.find(vid);
      if (it != bonus_mp.end()) {
        auto mi = it->second.first, mx = it->second.second;
        auto addm = rand() % (mx + 1 - mi) + mi;
        add_money(sp->getNameTag(), addm);
        char buf[64];
        snprintf(buf,64,L_KBONUS_KILL_TEXT,addm);//L_KBONUS_KILL_TEXT
        sendText(sp, string(buf));
        // sendText(sp, "you get $" + std::to_string() + " by killing");
      }
    }
  }
}

void CommandKillbonus::invoke(mandatory<KillbonusMode> mode) {
  auto out = &getOutput();
  switch (mode) {
  case KillbonusMode::Reload: load(out); break;
  case KillbonusMode::Debug: toggle_dbg(out); break;
  default: getOutput().error(L_KBONUS_KILL_UNKNOWN_MODE); return;//L_KBONUS_KILL_UNKNOWN_MODE
  }
}

void mod_init(std::list<string> &modlist) {
  load(nullptr);
  register_commands();
  reg_mobdie(handle_die);
  do_log("Loaded " BDL_TAG "");
  load_helper(modlist);
}