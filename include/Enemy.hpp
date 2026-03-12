# pragma once
# include "Common.hpp"

//------------------------------------------------------------
// Enemyクラス
// プレイヤーの敵となるキャラクターの挙動・状態を管理するクラス
//------------------------------------------------------------
class Enemy {
public:
	// コンストラクタ
	// @param pos: 初期座標
	Enemy(const Vec3& pos);

	// 更新処理
	// @param playerPos: プレイヤーの現在座標
	void update(const Vec3& playerPos);

	// 描画処理
	void draw() const;

	// デバッグ用の追加情報描画
	// @param font: 使用するフォント
	void drawDebug(const Font& font) const;

	// 敵からの攻撃判定を取得
	// Attackステートの特定タイミングでのみ攻撃判定が発生
	// @return 攻撃判定の当たり判定（Sphere）、攻撃中でなければnone
	Optional<Sphere> getAttackHitbox() const;

	// ダメージ処理
	// @param attackPos: 攻撃元の座標（ノックバック方向計算用）
	// @param damage: 受けるダメージ量
	void onHit(const Vec3& attackPos, int32 damage);

	// 敵キャラクターの当たり判定（Box）を取得
	Box getBox() const;

	// 敵が生存しているかどうか
	bool isAlive() const;

private:
	// 現在座標
	Vec3 m_pos;

	// 現在HP
	int32 m_hp;

	// 向いている角度（ラジアン）
	double m_angle;

	//-------------------------------
	// AI制御用の状態管理
	//-------------------------------

	// 現在のAIステート（Chase/Attack/Recover）
	EnemyState m_state;

	// ステート経過時間
	double m_stateTimer;

	//-------------------------------
	// 演出・エフェクト用
	//-------------------------------

	// 被弾時の点滅演出タイマー
	double m_flashTimer;

	// ノックバック速度ベクトル
	Vec3 m_knockbackVel;
};
