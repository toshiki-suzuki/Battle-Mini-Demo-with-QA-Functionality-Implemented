# pragma once
# include "Common.hpp"

// =========================================================
// 攻撃クラスのインターフェース
// =========================================================
// IAttackクラスは、すべての攻撃クラスが継承すべきインターフェースを定義します。
// 各攻撃クラスは、このクラスを基底クラスとして、攻撃の更新、描画、当たり判定を実装します。
class IAttack {
public:
	// 仮想デストラクタ（ポリモーフィズムをサポート）
	virtual ~IAttack() = default;

	// 攻撃の更新処理（継承先で実装）
	// @return 攻撃が有効であるかどうか（falseの場合、攻撃は終了）
	virtual bool update() = 0;

	// 攻撃の描画処理（継承先で実装）
	virtual void draw() const = 0;

	// 攻撃の当たり判定（Sphere型）を取得（継承先で実装）
	virtual Sphere getSphere() const = 0;

	// 攻撃がヒットしたかどうかを取得
	// @return ヒット状態（true: ヒット済み, false: 未ヒット）
	bool isHit() const { return m_hit; }

	// 攻撃をヒット状態に設定
	void setHit() { m_hit = true; }

protected:
	// 攻撃がヒットしたかどうかを管理するフラグ
	bool m_hit = false;
};

// =========================================================
// 個別の攻撃実装
// =========================================================

//------------------------------------------------------------
// Slashクラス
// プレイヤーの近接攻撃（斬撃）を表現するクラス
//------------------------------------------------------------
class Slash : public IAttack {
public:
	// コンストラクタ
	// @param pos: 攻撃の初期位置
	Slash(const Vec3& pos) : m_pos(pos), m_timer(0.0) {}

	// 攻撃の更新処理
	// @return 攻撃が有効であるかどうか（タイマーが一定時間を超えると無効）
	bool update() override {
		// 共通のDeltaTimeを使用（HitStopやTimeScaleに対応）
		m_timer += g_config.getDeltaTime();
		return m_timer < 0.3;
	}

	// 攻撃の描画処理
	void draw() const override {
		// 攻撃の進行度に応じたエフェクトを描画
		const double e = m_timer / 0.3;
		Cylinder{ m_pos, 4.0 * e, 0.2 }.draw(ColorF{ 1.0, 0.5, 0.0, 1.0 - e });

		// デバッグ表示：攻撃判定（赤色で描画）
		if (g_config.showHitboxes) getSphere().draw(ColorF{ 1.0, 0.0, 0.0, 0.3 });
	}

	// 攻撃の当たり判定（Sphere型）を取得
	// @return 攻撃の当たり判定（中心位置と半径）
	Sphere getSphere() const override { return Sphere{ m_pos, 4.0 }; }

private:
	// 攻撃の中心位置
	Vec3 m_pos;

	// 攻撃の経過時間を管理するタイマー
	double m_timer;
};

//------------------------------------------------------------
// Shotクラス
// プレイヤーの遠距離攻撃（射撃）を表現するクラス
//------------------------------------------------------------
class Shot : public IAttack {
public:
	// コンストラクタ
	// @param pos: 弾の初期位置
	// @param vel: 弾の速度ベクトル
	Shot(const Vec3& pos, const Vec3& vel) : m_pos(pos), m_velocity(vel) {}

	// 攻撃の更新処理
	// @return 攻撃が有効であるかどうか（弾が一定距離を超えると無効）
	bool update() override {
		// 弾の位置を速度に基づいて更新
		m_pos += m_velocity * g_config.getDeltaTime();
		return m_pos.length() < GroundSize;
	}

	// 攻撃の描画処理
	void draw() const override {
		// 弾のエフェクトを描画
		Sphere{ m_pos, 0.4 }.draw(Palette::Cyan);

		// デバッグ表示：攻撃判定（赤色で描画）
		if (g_config.showHitboxes) getSphere().draw(ColorF{ 1.0, 0.0, 0.0, 0.3 });
	}

	// 攻撃の当たり判定（Sphere型）を取得
	// @return 攻撃の当たり判定（中心位置と半径）
	Sphere getSphere() const override { return Sphere{ m_pos, 0.4 }; }

private:
	// 弾の現在位置
	Vec3 m_pos;

	// 弾の速度ベクトル
	Vec3 m_velocity;
};
