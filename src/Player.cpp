# include "Player.hpp"
# include "Attack.hpp"


//------------------------------------------------------------
// コンストラクタ
// プレイヤーオブジェクトの初期化を行います。
//------------------------------------------------------------
Player::Player() {}
Player::~Player() = default;

//------------------------------------------------------------
// 更新処理
// プレイヤーの状態を更新します。
// カメラ情報を受け取り、入力や状態遷移、攻撃の更新を行います。
// @param camera: 現在のカメラ情報
//------------------------------------------------------------
void Player::update(const BasicCamera3D& camera) {
	(void)camera; // 未使用警告(C4100)を回避

	if (m_state == PlayerState::Dead) return; // 死亡状態の場合は処理をスキップ

	double dt = g_config.getDeltaTime(); // フレーム間の経過時間を取得
	m_stateTimer += dt;

	// 無敵時間や回避クールダウンのタイマーを減少
	if (m_invincibleTimer > 0) m_invincibleTimer -= dt;
	if (m_dodgeInvincibleTimer > 0) m_dodgeInvincibleTimer -= dt;
	if (m_dodgeCooldown > 0) m_dodgeCooldown -= dt;

	// ノックバック処理
	if (m_knockbackVel.lengthSq() > 0.1) {
		m_pos += m_knockbackVel * dt;
		m_knockbackVel = Math::Lerp(m_knockbackVel, Vec3::Zero(), dt * 5.0);
	}

	// 入力処理と状態更新
	handleInput();
	updateState();

	// 攻撃の更新処理
	auto it = m_attacks.begin();
	while (it != m_attacks.end()) {
		if (!(*it)->update()) it = m_attacks.erase(it); // 攻撃が終了したら削除
		else ++it;
	}
}

//------------------------------------------------------------
// 入力処理
// プレイヤーの移動やアクション入力を処理します。
//------------------------------------------------------------
void Player::handleInput() {
	if (!isAlive()) return; // 生存していない場合は処理をスキップ

	const Vec3 camForward{ 0, 0, 1 }; // カメラの前方向
	const Vec3 camRight{ 1, 0, 0 };   // カメラの右方向
	Vec3 moveInput{ 0, 0, 0 };        // 移動入力ベクトル

	// キーボード入力
	if (KeyW.pressed()) moveInput += camForward;
	if (KeyS.pressed()) moveInput -= camForward;
	if (KeyA.pressed()) moveInput -= camRight;
	if (KeyD.pressed()) moveInput += camRight;

	// ゲームパッド入力
	if (const auto pad = XInput(0); pad.isConnected()) {
		double lx = pad.leftThumbX;
		double ly = pad.leftThumbY;
		if (Abs(lx) > 0.2) moveInput.x += lx;
		if (Abs(ly) > 0.2) moveInput.z += ly;
	}

	// 移動処理
	if (!moveInput.isZero() && m_state == PlayerState::Idle) {
		m_moveDir = moveInput.normalized();
		m_aimDir = m_moveDir;
		m_pos += m_moveDir * (15.0 * g_config.getDeltaTime());
		m_angle = Atan2(m_moveDir.x, m_moveDir.z);
	}

	// アクション入力
	const auto pad = XInput(0);
	if ((KeySpace.down() || pad.buttonB.down()) && m_dodgeCooldown <= 0.0) {
		m_inputBuffer = PlayerState::Dodge; // 回避
	}
	else if (MouseL.down() || pad.buttonX.down()) {
		m_inputBuffer = PlayerState::Slash; // 近接攻撃
	}
	else if (MouseR.down() || pad.buttonY.down()) {
		m_inputBuffer = PlayerState::Shot; // 遠距離攻撃
	}
}

//------------------------------------------------------------
// 状態更新処理
// プレイヤーの現在の状態を更新します。
//------------------------------------------------------------
void Player::updateState() {
	if (m_state == PlayerState::Idle && m_inputBuffer) {
		changeState(*m_inputBuffer); // 入力バッファに基づいて状態を変更
	}
	else if ((m_state == PlayerState::Slash || m_state == PlayerState::Shot) && m_stateTimer > 0.4) {
		changeState(PlayerState::Idle); // 攻撃終了後に待機状態に戻る
	}
	else if (m_state == PlayerState::Dodge) {
		if (m_stateTimer < 0.4) {
			m_pos += m_aimDir * (25.0 * g_config.getDeltaTime()); // 回避中の移動
		}
		else {
			changeState(PlayerState::Idle); // 回避終了後に待機状態に戻る
		}
	}
}

//------------------------------------------------------------
// 状態変更処理
// プレイヤーの状態を変更し、必要な初期化を行います。
// @param nextState: 次の状態
//------------------------------------------------------------
void Player::changeState(PlayerState nextState) {
	m_state = nextState;
	m_stateTimer = 0.0;
	m_inputBuffer = none;

	// 状態に応じた初期化処理
	if (nextState == PlayerState::Slash) {
		m_attacks << std::make_unique<Slash>(m_pos); // 近接攻撃を生成
	}
	else if (nextState == PlayerState::Shot) {
		Vec3 vel = Vec3{ Sin(m_angle), 0, Cos(m_angle) } *50.0;
		m_attacks << std::make_unique<Shot>(m_pos, vel); // 遠距離攻撃を生成
	}
	else if (nextState == PlayerState::Dodge) {
		m_dodgeInvincibleTimer = 0.2; // 回避無敵時間
		m_dodgeCooldown = 0.8;        // 回避クールダウン
	}
}

//------------------------------------------------------------
// 被弾処理
// プレイヤーがダメージを受けた際の処理を行います。
// @param sourcePos: 攻撃元の座標
// @param damage: 受けるダメージ量
// @param sourceName: 攻撃元の名前（デバッグ用）
//------------------------------------------------------------
void Player::takeDamage(const Vec3& sourcePos, int32 damage, const String& sourceName) {
	if (g_config.invincibleMode || isInvincible() || !isAlive()) return; // 無敵状態の場合は処理をスキップ

	m_hp -= damage; // ダメージを受ける
	m_invincibleTimer = 1.0; // 一定時間無敵状態にする

	m_lastHitBy = sourceName; // 最後に攻撃してきた相手の名前を記録
	m_lastDamage = damage;    // 最後に受けたダメージ量を記録
	g_config.addHitStop(0.1); // ヒットストップを追加

	if (m_hp <= 0) {
		m_state = PlayerState::Dead; // HPが0以下になったら死亡状態にする
		m_hp = 0;
	}
	else {
		// ノックバック処理
		Vec3 kbDir = (m_pos - sourcePos).setLength(1.0);
		kbDir.y = 0;
		m_knockbackVel = kbDir * 10.0;
	}
}

//------------------------------------------------------------
// リセット処理
// プレイヤーの状態を初期化します。
//------------------------------------------------------------
void Player::reset() {
	m_hp = m_maxHp;
	m_pos = { 0, 1, 0 };
	m_state = PlayerState::Idle;
	m_invincibleTimer = 0.0;
	m_knockbackVel = Vec3::Zero();
	m_lastHitBy = U"None";
	m_attacks.clear();
}

//------------------------------------------------------------
// 描画処理
// プレイヤーを描画します。
//------------------------------------------------------------
void Player::draw() const {
	if (m_state == PlayerState::Dead) return; // 死亡状態の場合は描画をスキップ

	// 無敵状態の点滅演出
	if (m_invincibleTimer > 0 && Periodic::Triangle0_1(0.2s) < 0.5) return;

	ColorF pColor = Palette::White;
	if (m_state == PlayerState::Dodge) pColor = Palette::Lightskyblue; // 回避中の色
	if (m_dodgeInvincibleTimer > 0) pColor.a = 0.5; // 無敵中の透明度

	Sphere{ m_pos, 1.0 }.draw(pColor); // プレイヤーの本体を描画

	// 向いている方向を示すマーカーを描画
	Vec3 forwardMarker = m_pos + m_aimDir * 1.2;
	Sphere{ forwardMarker, 0.3 }.draw(Palette::Orange);

	// デバッグ用の当たり判定を描画
	if (g_config.showHitboxes) getHurtbox().draw(ColorF{ 0.0, 0.5, 1.0, 0.3 });

	// 攻撃の描画
	for (const auto& attack : m_attacks) attack->draw();
}

//------------------------------------------------------------
// デバッグ描画処理
// プレイヤーのデバッグ情報を描画します。
// @param font: 使用するフォント
//------------------------------------------------------------
void Player::drawDebug(const Font& font) const {
	// --------------------------------------------------
	// 常に表示するUI（HPバーなど）
	// --------------------------------------------------
	const Vec2 hpBarPos{ 30, 30 };
	RectF{ hpBarPos, 400, 35 }.draw(ColorF{ 0.1, 0.8 });
	const double hpRate = static_cast<double>(Max(0, m_hp)) / static_cast<double>(m_maxHp);
	const ColorF barColor = (m_hp <= 1) ? ColorF{ Palette::Red } : ColorF{ Palette::Cyan };
	RectF{ hpBarPos, 400 * hpRate, 35 }.draw(barColor);

	font(Format(U"HP: ", m_hp, U" / ", m_maxHp)).draw(hpBarPos.movedBy(10, 2), Palette::White);

	// デバッグパネルがOFFの時は、キー操作のヒントを出しておく
	if (!g_config.showDebugPanel) {
		font(U"Press [Tab] for Debug Menu").draw(16, Vec2{ 30, 75 }, Palette::Lightgray);
		return; // これ以降のデバッグパネル描画処理を行わずに終了する
	}

	// --------------------------------------------------
	// F1キーで切り替えられるデバッグパネル（開発用情報）
	// --------------------------------------------------
	const double sidePanelWidth = 340;
	const RectF sidePanel{ Scene::Width() - sidePanelWidth - 20, 20, sidePanelWidth, Scene::Height() - 40 };
	sidePanel.draw(ColorF{ 0, 0.6 });

	auto drawSection = [&](const String& title, const Array<String>& lines, double yOffset) {
		const double sectionHeight = static_cast<double>(lines.size() + 1) * 25.0 + 15.0;
		const RectF sectionRect{ sidePanel.x + 10, sidePanel.y + yOffset, sidePanel.w - 20, sectionHeight };
		sectionRect.draw(ColorF{ 0.1, 0.7 }).drawFrame(1, 0, Palette::Gray);
		font(title).draw(18, sectionRect.pos.movedBy(10, 5), Palette::Orange);
		for (size_t i = 0; i < lines.size(); ++i) {
			font(lines[i]).draw(14, sectionRect.pos.movedBy(10, 35 + static_cast<double>(i) * 25.0), Palette::White);
		}
		return sectionHeight + 10.0;
	};

	double currentY = 10;

	String padInfo = XInput(0).isConnected() ? U"Gamepad: Connected" : U"Gamepad: None";
	currentY += drawSection(U"System & Debug Controls", {
		Format(U"FPS: ", Profiler::FPS(), U" | TimeScale: ", g_config.timeScale),
		padInfo,
		U"[H] Show Hitboxes: " + String(g_config.showHitboxes ? U"ON" : U"OFF"),
		U"[I] Invincible Cheat: " + String(g_config.invincibleMode ? U"ON" : U"OFF"),
		U"[T] Slow Motion Toggle",
		U"[E] Spawn Enemy"
	}, currentY);

	String stateStr = (m_state == PlayerState::Dodge) ? U"Dodge" :
		(m_state == PlayerState::Slash) ? U"Slash" :
		(m_state == PlayerState::Shot) ? U"Shot" :
		(m_state == PlayerState::Idle) ? U"Idle" : U"Dead";

	currentY += drawSection(U"Combat QA Info", {
		Format(U"State: ", stateStr),
		Format(U"i-frame: ", Max(0.0, m_dodgeInvincibleTimer), U"s left (Dodge)"),
		Format(U"Invincible: ", Max(0.0, m_invincibleTimer), U"s left (Hit)"),
		Format(U"Last Hit By: ", m_lastHitBy, U" (-", m_lastDamage, U")"),
		Format(U"HitStop: ", Max(0.0, g_config.hitStopTimer), U"s")
	}, currentY);
}
