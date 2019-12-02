#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"

PLUGIN_API const char PLUGIN_PRINT_NAME[32] = "Mundo";
PLUGIN_API const char PLUGIN_PRINT_AUTHOR[32] = "BiggieSmalls";
PLUGIN_API ChampionId PLUGIN_TARGET_CHAMP = ChampionId::DrMundo;

namespace Menu
{
	IMenu* MenuInstance = nullptr;

	namespace Combo
	{
		IMenuElement* Enabled = nullptr;
		IMenuElement* UseQ = nullptr;
		IMenuElement* UseW = nullptr;
		IMenuElement* UseE = nullptr;
		IMenuElement* UseR = nullptr;
		IMenuElement* RHP = nullptr;
	}

	namespace Harass
	{
		IMenuElement* Enabled = nullptr;
		IMenuElement* UseQ = nullptr;
		IMenuElement* UseE = nullptr;
		IMenuElement* TowerHarass = nullptr;
		IMenuElement* ManaQ = nullptr;
	}

	namespace LaneClear
	{
		IMenuElement* Enabled = nullptr;
		IMenuElement* UseQ = nullptr;
		IMenuElement* UseW = nullptr;
		IMenuElement* UseE = nullptr;
		IMenuElement* ManaClear = nullptr;
		IMenuElement* MinMinions = nullptr;
	}

	namespace Killsteal
	{
		IMenuElement* Enabled = nullptr;
		IMenuElement* UseQ = nullptr;
	}

	namespace Misc
	{
		IMenuElement* Antigap = nullptr;
		IMenuElement* WOnCc = nullptr;
		IMenuElement* WToggle = nullptr;
	}
	namespace Drawings
	{
		IMenuElement* Toggle = nullptr;
		IMenuElement* DrawQRange = nullptr;
		IMenuElement* DrawWRange = nullptr;
		IMenuElement* DrawERange = nullptr;
	}

	namespace Colors
	{
		IMenuElement* QColor = nullptr;
		IMenuElement* WColor = nullptr;
		IMenuElement* EColor = nullptr;
	}
}

namespace Spells
{
	std::shared_ptr<ISpell> Q = nullptr;
	std::shared_ptr<ISpell> W = nullptr;
	std::shared_ptr<ISpell> E = nullptr;
	std::shared_ptr<ISpell> R = nullptr;
}

//number of  enemies in Range
int CountEnemiesInRange(const Vector& Position, const float Range)
{
	auto Enemies = g_ObjectManager->GetChampions(false);
	int Counter = 0;
	for (auto& Enemy : Enemies)
		if (Enemy->IsVisible() && Enemy->IsValidTarget() && Position.Distance(Enemy->Position()) <= Range)
			Counter++;


	auto OtherObjects = g_ObjectManager->GetByType(EntityType::AIMinionClient);
	for (auto OtherObject : OtherObjects)
	{
		if (!OtherObject->IsAlly() && Position.Distance(OtherObject->Position()) <= Range)
			Counter++;
	}

	return Counter;
}

// returns prediction of spell
auto get_prediction(std::shared_ptr<ISpell> spell, IGameObject* unit) -> IPredictionOutput {
	return g_Common->GetPrediction(unit, spell->Range(), spell->Delay(), spell->Radius(), spell->Speed(), spell->CollisionFlags(),
		g_LocalPlayer->ServerPosition());
}

// killsteal
void KillstealLogic()
{
	const auto Enemies = g_ObjectManager->GetChampions(false);
	for (auto Enemy : Enemies)
	{
		if (Menu::Killsteal::UseQ->GetBool() && Spells::Q->IsReady() && Enemy->IsValidTarget(Spells::Q->Range()))
		{
			auto QDamage = g_Common->GetSpellDamage(g_LocalPlayer, Enemy, SpellSlot::W, false);
			if (QDamage >= Enemy->RealHealth(false, true))
				Spells::Q->Cast(Enemy, HitChance::High);
		}
	}
}

// combo
void ComboLogic()
{


	if (Menu::Combo::Enabled->GetBool())
	{
		// Q  max range ## add fast pred if we fighting super close
		if (Menu::Combo::UseQ->GetBool() && Spells::Q->IsReady())
		{
			auto Target = g_Common->GetTarget(Spells::Q->Range(), DamageType::Magical);
			if (Target && Target->IsValidTarget())
			{
				Spells::Q->Cast(Target, HitChance::VeryHigh);
			}
		}

		// R logic
		if (Menu::Combo::UseR && Spells::R->IsReady())
		{
			if (CountEnemiesInRange(g_LocalPlayer->Position(), 400.f) >= 1 && g_LocalPlayer->HealthPercent() <= Menu::Combo::RHP->GetInt())
				Spells::R->Cast();
		}

		// W in combo ### toggle spell. turn off if nothing in range
		if (Menu::Combo::UseW->GetBool() && Spells::W->IsReady() && !g_LocalPlayer->HasBuff("BurningAgony") && g_LocalPlayer->HealthPercent() >= 20)
		{
			auto Target = g_Common->GetTarget(Spells::W->Range(), DamageType::Magical);
			if (Target && Target->IsValidTarget())
			{
				Spells::W->Cast();
			}
		}		
	}
}

// harass
void HarassLogic()
{
	if (Menu::Harass::TowerHarass->GetBool() && g_LocalPlayer->IsUnderMyEnemyTurret())
		return;

	if (g_LocalPlayer->HealthPercent() < Menu::Harass::ManaQ->GetInt())
		return;

	if (Menu::Harass::Enabled->GetBool())
	{
		// Q  max range ## add fast pred if we fighting super close
		if (Menu::Harass::UseQ->GetBool() && Spells::Q->IsReady())
		{
			auto Target = g_Common->GetTarget(Spells::Q->Range(), DamageType::Magical);
			if (Target && Target->IsValidTarget())
			{
				Spells::Q->Cast(Target, HitChance::VeryHigh);
			}
		}
		//// E cast on target that is not close enough to reset AA
		//if (Menu::Harass::UseE->GetBool() && Spells::E->IsReady())
		//{
		//	auto Target = g_Common->GetTarget(Spells::E->Range(), DamageType::Physical);
		//	if (Target && Target->IsValidTarget() && Target->Distance(g_LocalPlayer) >= 190.f)

		//		Spells::E->Cast();
		//}
	}
}
// E after AA
void OnAfterAttack(IGameObject* target)
{
	{
		if (Menu::Combo::Enabled && Menu::Combo::UseE->GetBool() && Spells::E->IsReady())

		{
			const auto OrbwalkerTarget = g_Orbwalker->GetTarget();
			if (OrbwalkerTarget && OrbwalkerTarget->IsAIHero() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
			{
				Spells::E->Cast();
				g_Orbwalker->ResetAA();
			}
		}
	}
	{
		if (Menu::Harass::Enabled && Menu::Harass::UseE->GetBool() && Spells::E->IsReady())
		{
			const auto OrbwalkerTarget = g_Orbwalker->GetTarget();
			if (OrbwalkerTarget && OrbwalkerTarget->IsAIHero() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass))
			{
				Spells::E->Cast();
				g_Orbwalker->ResetAA();
			}
		}
	}
	{
		if (Menu::LaneClear::Enabled && Menu::LaneClear::UseE->GetBool() && Spells::E->IsReady())

		{
			const auto OrbwalkerTarget = g_Orbwalker->GetTarget();
			if (OrbwalkerTarget && OrbwalkerTarget->IsMonster()  && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))
			{
				Spells::E->Cast();
				g_Orbwalker->ResetAA();
			}
		}
	}
}

void MiscLogic()
{

	// Dashing and Immobile 
	const auto Enemies = g_ObjectManager->GetChampions(false);
	for (auto Enemy : Enemies)
	{
		if (Spells::Q->IsReady())
		{
			if (Enemy && Enemy->IsValidTarget(Spells::Q->Range()))
			{
				const auto pred = get_prediction(Spells::Q, Enemy);
				// Cast Q on Dashing
				if (Menu::Misc::Antigap->GetBool())
					if (pred.Hitchance == HitChance::Dashing)
						Spells::Q->Cast(pred.CastPosition);
				//Cast Q on hard CC
				if (pred.Hitchance == HitChance::Immobile)
					Spells::Q->Cast(pred.CastPosition);
			}
		}
		//W Toggle
		if (Menu::Misc::WToggle->GetBool() && g_LocalPlayer->HasBuff("BurningAgony"))
		{
			if ((CountEnemiesInRange(g_LocalPlayer->Position(), 500.f) < 1) || g_LocalPlayer->HealthPercent() <= 30)
				Spells::W->Cast();
		}
		 // W on CC
		if (Menu::Misc::WOnCc && Spells::W->IsReady() && Enemy->IsInRange(450.f) && !g_LocalPlayer->HasBuff("BurningAgony"))
		{
			if (g_LocalPlayer->HasBuffOfType(BuffType::Charm) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Fear) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Slow) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Knockup) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Polymorph) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Snare) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Stun) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Suppression) ||
				g_LocalPlayer->HasBuffOfType(BuffType::Taunt))
			Spells::W->Cast();
		}		
	}

	//Auto kill monsters with Q
	{
		const auto Enemies = g_ObjectManager->GetJungleMobs();
		for (auto Enemy : Enemies)
		{
			auto QDamage = g_Common->GetSpellDamage(g_LocalPlayer, Enemy, SpellSlot::Q, false);
			if (Enemy && Enemy->IsInRange(Spells::Q->Range()) && Spells::Q->IsReady() && QDamage >= Enemy->RealHealth(false, true) && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))
				Spells::Q->Cast(Enemy, HitChance::High);
		}
	}
}

// Lane Clear Logic
void LaneCLearLogic()
{
	if (g_LocalPlayer->HealthPercent() < Menu::LaneClear::ManaClear->GetInt())
		return;

	auto MinMinions = Menu::LaneClear::MinMinions->GetInt();
	if (!MinMinions)
		return;

	const auto Targets = g_ObjectManager->GetMinionsEnemy();
	for (auto Target : Targets)

	{
		if (Menu::LaneClear::UseQ->GetBool())
		{
			auto QDamage = g_Common->GetSpellDamage(g_LocalPlayer, Target, SpellSlot::Q, false);
			if (Target && Target->IsMinion() && Target->IsInRange(Spells::Q->Range()) && Target->Distance(g_LocalPlayer) >= 250.f && Spells::Q->IsReady() && QDamage >= Target->RealHealth(false, true))
				Spells::Q->Cast(Target, HitChance::High);
		}
		if (Menu::LaneClear::UseW->GetBool())
		{
			if (Target && Target->IsMinion() && Target->IsInRange(Spells::W->Range()) && Spells::W->IsReady() && !g_LocalPlayer->HasBuff("BurningAgony"))
				Spells::W->CastOnBestFarmPosition(MinMinions);
		}
		if (Menu::LaneClear::UseE->GetBool() && Target->Distance(g_LocalPlayer) >= 190.f)
		{
			auto EDamage = g_Common->GetSpellDamage(g_LocalPlayer, Target, SpellSlot::E, false) + g_LocalPlayer->AutoAttackDamage(Target, true);
			if (Target && Target->IsMinion() && Target->IsInRange(Spells::E->Range()) && Spells::E->IsReady() && EDamage >= Target->RealHealth(true, false))
			{
				Spells::E->Cast();
				g_Orbwalker->ResetAA();
			}
		}
	}

	// Jungle Clear Logic
	{
		const auto Monsters = g_ObjectManager->GetJungleMobs();
		for (auto Monster : Monsters)
		{
			if (Monster && Monster->IsMonster() && Menu::LaneClear::UseQ->GetBool() && Spells::Q->IsReady() && Monster->IsInRange(Spells::Q->Range()))
				Spells::Q->Cast(Monster, HitChance::VeryHigh);

			if (Monster && Monster->IsMonster() && Spells::W->IsReady() && Menu::LaneClear::UseW->GetBool() && Monster->IsInRange(Spells::W->Range()) && !g_LocalPlayer->HasBuff("BurningAgony"))
				Spells::W->Cast(Monster, HitChance::High);

			//if (Monster && Monster->IsMonster() && Spells::E->IsReady() && Menu::LaneClear::UseE->GetBool() && Monster->IsInRange(Spells::E->Range()))
			//		Spells::E->Cast();
		}
	}
}

void OnGameUpdate()
{
	if (g_LocalPlayer->IsDead())
		return;


	if (Menu::Killsteal::Enabled->GetBool())
		KillstealLogic();

	if (Menu::Combo::Enabled->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
		ComboLogic();

	if (Menu::Harass::Enabled->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass))
		HarassLogic();

	if (Menu::LaneClear::Enabled->GetBool() && g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))
		LaneCLearLogic();

	MiscLogic();


}

// Drawings
void OnHudDraw()
{
	if (!Menu::Drawings::Toggle->GetBool() || g_LocalPlayer->IsDead())
		return;

	const auto PlayerPosition = g_LocalPlayer->Position();
	const auto CirclesWidth = 1.5f;

	if (Menu::Drawings::DrawQRange->GetBool() && !Spells::Q->CooldownTime())
		g_Drawing->AddCircle(PlayerPosition, Spells::Q->Range(), Menu::Colors::QColor->GetColor(), CirclesWidth);

	if (Menu::Drawings::DrawWRange->GetBool() && !Spells::W->CooldownTime())
		g_Drawing->AddCircle(PlayerPosition, Spells::W->Range(), Menu::Colors::WColor->GetColor(), CirclesWidth);

	if (Menu::Drawings::DrawERange->GetBool() && !Spells::E->CooldownTime())
		g_Drawing->AddCircle(PlayerPosition, Spells::E->Range(), Menu::Colors::EColor->GetColor(), CirclesWidth);
}

PLUGIN_API bool OnLoadSDK(IPluginsSDK* plugin_sdk)
{
	DECLARE_GLOBALS(plugin_sdk);

	if (g_LocalPlayer->ChampionId() != ChampionId::DrMundo)
		return false;

	using namespace Menu;
	using namespace Spells;

	MenuInstance = g_Menu->CreateMenu("Mundo", "Mundo by BiggieSmalls");

	const auto ComboSubMenu = MenuInstance->AddSubMenu("Combo", "combo_menu");
	Menu::Combo::Enabled = ComboSubMenu->AddCheckBox("Enable Combo", "enable_combo", true);
	Menu::Combo::UseQ = ComboSubMenu->AddCheckBox("Use Q", "combo_use_q", true);
	Menu::Combo::UseW = ComboSubMenu->AddCheckBox("Use W", "combo_use_w", true);
	Menu::Combo::UseE = ComboSubMenu->AddCheckBox("Use E", "combo_use_e", true);
	Menu::Combo::UseR = ComboSubMenu->AddCheckBox("Use R", "combo_use_R", true);
	Menu::Combo::RHP = ComboSubMenu->AddSlider("Use R on % health", "min_health_range_r", 40, 0, 100);

	const auto HarassSubMenu = MenuInstance->AddSubMenu("Harass", "harass_menu");
	Menu::Harass::Enabled = HarassSubMenu->AddCheckBox("Enable Harass", "enable_harass", true);
	Menu::Harass::UseQ = HarassSubMenu->AddCheckBox("Use Q", "harass_use_q", true);
	Menu::Harass::UseE = HarassSubMenu->AddCheckBox("Use E", "harass_use_ea", false);
	Menu::Harass::TowerHarass = HarassSubMenu->AddCheckBox("Don't Harass under tower", "use_tower_q", true);
	Menu::Harass::ManaQ = HarassSubMenu->AddSlider("Min Health", "min_mana_harass", 60, 0, 100, true);

	const auto LaneClearSubMenu = MenuInstance->AddSubMenu("Lane Clear", "laneclear_menu");
	Menu::LaneClear::Enabled = LaneClearSubMenu->AddCheckBox("Enable Lane Clear", "enable_laneclear", true);
	Menu::LaneClear::UseQ = LaneClearSubMenu->AddCheckBox("Use Q", "laneclear_use_q", false);
	Menu::LaneClear::UseW = LaneClearSubMenu->AddCheckBox("Use W", "laneclear_use_w", true);
	Menu::LaneClear::UseE = LaneClearSubMenu->AddCheckBox("Use E", "laneclear_use_e", true);
	Menu::LaneClear::ManaClear = LaneClearSubMenu->AddSlider("Min Health", "min_mana_laneclear", 80, 0, 100, true);
	Menu::LaneClear::MinMinions = LaneClearSubMenu->AddSlider("Min minions for W", "lane_clear_min_minions", 3, 0, 9);

	const auto KSSubMenu = MenuInstance->AddSubMenu("KS", "ks_menu");
	Menu::Killsteal::Enabled = KSSubMenu->AddCheckBox("Enable Killsteal", "enable_ks", true);
	Menu::Killsteal::UseQ = KSSubMenu->AddCheckBox("Use Q", "q_ks", true);

	const auto MiscSubMenu = MenuInstance->AddSubMenu("Misc", "misc_menu");
	Menu::Misc::Antigap = MiscSubMenu->AddCheckBox("Antigapcloser Q", "q_dashing", true);
	Menu::Misc::WOnCc = MiscSubMenu->AddCheckBox("Auto W on CC", "q_hard_cc", true);
	Menu::Misc::WToggle = MiscSubMenu->AddCheckBox("Auto Turn off W", "w_toggle_cc", true);

	const auto DrawingsSubMenu = MenuInstance->AddSubMenu("Drawings", "drawings_menu");
	Drawings::Toggle = DrawingsSubMenu->AddCheckBox("Enable Drawings", "drawings_toggle", true);
	Drawings::DrawQRange = DrawingsSubMenu->AddCheckBox("Draw Q Range", "draw_q", true);
	Drawings::DrawWRange = DrawingsSubMenu->AddCheckBox("Draw W Range", "draw_w", true);
	Drawings::DrawERange = DrawingsSubMenu->AddCheckBox("Draw E Range", "draw_e", true);

	const auto ColorsSubMenu = DrawingsSubMenu->AddSubMenu("Colors", "color_menu");
	Colors::QColor = ColorsSubMenu->AddColorPicker("Q Range", "color_q_range", 0, 175, 255, 180);
	Colors::WColor = ColorsSubMenu->AddColorPicker("W Range", "color_w_range", 0, 215, 155, 180);
	Colors::EColor = ColorsSubMenu->AddColorPicker("E Range", "color_e_range", 0, 75, 55, 180);

	Spells::Q = g_Common->AddSpell(SpellSlot::Q, 975.f);
	Spells::W = g_Common->AddSpell(SpellSlot::W, 290.f);
	Spells::E = g_Common->AddSpell(SpellSlot::E, 225.f);
	Spells::R = g_Common->AddSpell(SpellSlot::R);

	// pred hitchance is very good with these weird values
	Spells::Q->SetSkillshot(0.25f, 75.f, 1700.f, kCollidesWithMinions && kCollidesWithHeroes && kCollidesWithYasuoWall, kSkillshotLine);

	EventHandler<Events::GameUpdate>::AddEventHandler(OnGameUpdate);
	EventHandler<Events::OnAfterAttackOrbwalker>::AddEventHandler(OnAfterAttack);
	EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	//	EventHandler<Events::OnBuff>::AddEventHandler(OnBuffChange);
	//EventHandler<Events::OnCreateObject>::AddEventHandler(OnCreateObject);
	//EventHandler<Events::OnProcessSpellCast>::AddEventHandler(OnProcessSpell);

	g_Common->ChatPrint("<font color='#FFC300'>Mundo Loaded!</font>");
	g_Common->Log("Mundo plugin loaded.");

	return true;
}

PLUGIN_API void OnUnloadSDK()
{
	Menu::MenuInstance->Remove();
	EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
	EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
	EventHandler<Events::OnAfterAttackOrbwalker>::RemoveEventHandler(OnAfterAttack);
	EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	//	EventHandler<Events::OnBuff>::RemoveEventHandler(OnBuffChange);
	//EventHandler<Events::OnCreateObject>::RemoveEventHandler(OnCreateObject);
	//EventHandler<Events::OnProcessSpellCast>::RemoveEventHandler(OnProcessSpell);

	g_Common->ChatPrint("<font color='#00BFFF'>Mundo Unloaded.</font>");
}