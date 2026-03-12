# include "Enemy.hpp"

//------------------------------------------------------------
// コンストラクタ
// 敵キャラクターの初期化を行います。
// @param pos: 敵の初期位置
//------------------------------------------------------------
Enemy::Enemy(const Vec3& pos)
	: m_pos(pos), m_hp(3), m_angle(0.0)
	, m_state(EnemyState::Chase), m_stateTimer(0.0)
	, m_flashTimer(0.0), m_knockbackVel(0, 0, 0)
{
}

//------------------------------------------------------------
// 更新処理
// 敵キャラクターの状態を更新します。
// プレイヤーの位置に応じて行動を変化させます。
// @param playerPos: プレイヤーの現在位置
//------------------------------------------------------------
void Enemy::update(const Vec3& playerPos) {
	if (!isAlive()) return; // 敵が生存していない場合は処理をスキップ

	double dt = g_config.getDeltaTime(); // フレーム間の経過時間を取得

	// 被弾時の点滅タイマーを減少
	if (m_flashTimer > 0) m_flashTimer -= dt;

	// ノックバック処理
	if (m_knockbackVel.lengthSq() > 0.1) {
		m_pos += m_knockbackVel * dt;
		m_knockbackVel = Math::Lerp(m_knockbackVel, Vec3::Zero(), dt * 5.0);
	}

	// プレイヤーとの距離と方向を計算
	Vec3 diff = playerPos - m_pos;
	diff.y = 0; // 高さ方向の差を無視
	double dist = diff.length();
	if (dist > 0.001) m_angle = Atan2(diff.x, diff.z); // プレイヤー方向を向く

	m_stateTimer += dt; // 現在の状態の経過時間を更新

	// 現在の状態に応じた処理
	switch (m_state) {
	case EnemyState::Chase: // プレイヤーを追いかける状態
		if (dist > 2.5) {
			m_pos += diff.setLength(4.0 * dt); // プレイヤーに向かって移動
		}
		else {
			m_state = EnemyState::Attack; // 一定距離内に入ったら攻撃状態に移行
			m_stateTimer = 0.0;
		}
		break;

	case EnemyState::Attack: // 攻撃状態
		if (m_stateTimer > 0.6) {
			m_state = EnemyState::Recover; // 攻撃後は回復状態に移行
			m_stateTimer = 0.0;
		}
		break;

	case EnemyState::Recover: // 回復状態（次の行動までの待機）
		if (m_stateTimer > 1.0) {
			m_state = EnemyState::Chase; // 再び追跡状態に戻る
			m_stateTimer = 0.0;
		}
		break;
	}
}

//------------------------------------------------------------
// 描画処理
// 敵キャラクターを描画します。
// 状態に応じて色やエフェクトを変更します。
//------------------------------------------------------------
void Enemy::draw() const {
	if (!isAlive()) return; // 敵が生存していない場合は描画をスキップ

	// 状態に応じた色を設定
	ColorF color = (m_flashTimer > 0.0) ? Palette::White : Palette::Red;
	if (m_state == EnemyState::Attack) color = Palette::Darkred;

	// 敵の本体を描画
	Box{ m_pos, 2.0 }.draw(color);

	// 敵の向いている方向を示すマーカーを描画
	Vec3 forward = m_pos + Vec3{ Sin(m_angle), 0, Cos(m_angle) } *1.5;
	Sphere{ forward, 0.4 }.draw(Palette::Yellow);

	// デバッグ用の当たり判定を描画
	if (g_config.showHitboxes) {
		getBox().drawFrame(ColorF{ 0.0, 0.5, 1.0, 0.5 });
		if (m_state == EnemyState::Chase) {
			Sphere{ m_pos, 2.5 }.draw(ColorF{ 1.0, 1.0, 0.0, 0.1 });
		}
	}
}

//------------------------------------------------------------
// 攻撃判定の取得
// 攻撃状態の特定タイミングでのみ攻撃判定を返します。
// @return 攻撃判定（Sphere型）、攻撃中でなければnone
//------------------------------------------------------------
Optional<Sphere> Enemy::getAttackHitbox() const {
	if (m_state == EnemyState::Attack && m_stateTimer >= 0.3 && m_stateTimer <= 0.5) {
		Vec3 attackPos = m_pos + Vec3{ Sin(m_angle), 0, Cos(m_angle) } *1.5;
		return Sphere{ attackPos, 1.5 }; // 攻撃範囲を表すSphereを返す
	}
	return none; // 攻撃中でない場合はnoneを返す
}

//------------------------------------------------------------
// ダメージ処理
// 敵が攻撃を受けた際の処理を行います。
// @param attackPos: 攻撃元の座標（ノックバック方向計算用）
// @param damage: 受けるダメージ量
//------------------------------------------------------------
void Enemy::onHit(const Vec3& attackPos, int32 damage) {
	if (m_hp <= 0) return; // 既にHPが0以下の場合は処理をスキップ
	m_hp -= damage; // ダメージを受ける

	m_flashTimer = 0.1; // 被弾時の点滅演出を開始

	// ノックバック方向を計算
	Vec3 kbDir = (m_pos - attackPos).setLength(1.0);
	kbDir.y = 0; // 高さ方向のノックバックを無視
	m_knockbackVel = kbDir * 15.0; // ノックバック速度を設定
}

//------------------------------------------------------------
// 当たり判定の取得
// 敵キャラクターの当たり判定（Box型）を返します。
// @return 敵の当たり判定（Box型）
//------------------------------------------------------------
Box Enemy::getBox() const { return Box{ m_pos, 2.0 }; }

//------------------------------------------------------------
// 生存状態の確認
// 敵キャラクターが生存しているかどうかを返します。
// @return true: 生存, false: 死亡
//------------------------------------------------------------
bool Enemy::isAlive() const { return m_hp > 0; }
