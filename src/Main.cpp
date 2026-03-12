# include "Player.hpp"
# include "Enemy.hpp"
# include "Attack.hpp"

//------------------------------------------------------------
// メイン関数
// ゲームのエントリーポイント。
// ゲームの初期化、メインループ、状態遷移、描画処理を行います。
//------------------------------------------------------------
void Main() {
	// ウィンドウサイズを設定
	Window::Resize(1280, 720);

	// フォントの設定
	const Font font{ 25, Typeface::Medium }; // 通常フォント
	const Font titleFont{ 60, Typeface::Heavy }; // タイトル用フォント

	// プレイヤーと敵の初期化
	Player player;
	Array<Enemy> enemies;

	// ゲームの初期状態を「タイトル画面」に設定
	GameState gameState = GameState::Title;

	// カメラの初期設定
	BasicCamera3D camera{ Scene::Size(), 45_deg, Vec3{ 0, 10, -20 }, Vec3{ 0, 0, 0 } };

	// ゲーム開始時のセットアップ処理
	auto setupGame = [&]() {
		player.reset(); // プレイヤーの状態をリセット
		enemies.clear(); // 敵リストをクリア
		for (int i = 0; i < 3; ++i) {
			// ランダムな位置に敵を生成
			enemies << Enemy{ Vec3{ Random(-20.0, 20.0), 1.0, Random(10.0, 30.0) } };
		}
	};

	// メインループ
	while (System::Update()) {
		// デバッグ設定の更新
		g_config.update();

		// ゲーム状態に応じた処理を分岐
		switch (gameState) {
		case GameState::Title: // タイトル画面
		{
			// 背景色を設定
			Scene::SetBackground(ColorF{ 0.1, 0.2, 0.3 });

			// タイトルテキストを描画
			titleFont(U"3D ACTION GAME").drawAt(Scene::CenterF().movedBy(0, -50), Palette::White);

			// 点滅するテキストの演出
			font(U"Press [ENTER] to Start").drawAt(Scene::CenterF().movedBy(0, 50), ColorF{ 1.0, Periodic::Sine0_1(2s) });

			// EnterキーまたはゲームパッドのSTARTボタンでゲーム開始
			if (KeyEnter.down() || XInput(0).buttonStart.down()) {
				setupGame(); // ゲームの初期化
				gameState = GameState::Play; // ゲームプレイ状態に遷移
			}
			break;
		}
		case GameState::Play: // ゲームプレイ中
		{
			// ==========================================
			// デバッグシステム・QAツールの入力処理
			// ==========================================
			// デバッグパネルの表示切り替え
			if (KeyTab.down()) g_config.showDebugPanel = !g_config.showDebugPanel;

			// デバッグ機能のショートカットキー
			if (KeyH.down()) g_config.showHitboxes = !g_config.showHitboxes; // 当たり判定の可視化
			if (KeyI.down()) g_config.invincibleMode = !g_config.invincibleMode; // 無敵モード
			if (KeyT.down()) g_config.timeScale = (g_config.timeScale == 1.0) ? 0.2 : 1.0; // スローモーション切り替え
			if (KeyE.down()) enemies << Enemy{ player.getPos() + Vec3{Random(-10.0,10.0), 0, Random(10.0,20.0)} }; // 敵を追加

			// ==========================================
			// ゲームロジックの更新
			// ==========================================
			// カメラの視点をプレイヤーに追従させる
			Vec3 targetEye = player.getPos() + Vec3{ 0, 4.0, -8.0 }; // カメラの位置
			Vec3 targetFocus = player.getPos() + Vec3{ 0, 1.0, 2.0 }; // カメラの注視点
			camera.setView(targetEye, targetFocus);

			// プレイヤーの更新処理
			player.update(camera);

			// 敵の更新処理
			for (auto& enemy : enemies) {
				enemy.update(player.getPos()); // プレイヤーの位置を基に敵を更新
				if (auto enemyHitbox = enemy.getAttackHitbox()) { // 敵の攻撃判定を取得
					if (g_config.showHitboxes) enemyHitbox->draw(ColorF{ 1.0, 0.0, 0.0, 0.5 }); // 攻撃判定を可視化
					if (player.isAlive() && enemyHitbox->intersects(player.getHurtbox())) {
						// プレイヤーが攻撃を受けた場合の処理
						player.takeDamage(enemy.getBox().center, 1, U"Enemy Melee");
					}
				}
			}

			// プレイヤーの攻撃と敵の当たり判定の処理
			for (auto& attack : player.getAttacks()) {
				if (attack->isHit()) continue; // 既にヒットした攻撃はスキップ
				for (auto& enemy : enemies) {
					if (enemy.isAlive() && attack->getSphere().intersects(enemy.getBox())) {
						// 攻撃が敵にヒットした場合の処理
						enemy.onHit(player.getPos(), 1);
						attack->setHit();
						g_config.addHitStop(0.05); // ヒットストップを追加
						break;
					}
				}
			}

			// 倒した敵をリストから削除
			enemies.remove_if([](const Enemy& e) { return !e.isAlive(); });

			// ==========================================
			// 描画処理
			// ==========================================
			{
				Graphics3D::SetCameraTransform(camera); // カメラの設定
				Scene::SetBackground(ColorF{ 0.1, 0.1, 0.15 }); // 背景色を設定
				Plane{ Vec3{0, 0, 0}, GroundSize }.draw(ColorF{ 0.4 }); // 地面を描画

				player.draw(); // プレイヤーを描画
				for (const auto& enemy : enemies) enemy.draw(); // 敵を描画
			}

			// UIの描画
			player.drawDebug(font);

			// デバッグパネルが非表示の時でもFPSを表示
			if (!g_config.showDebugPanel) {
				font(Format(U"FPS: ", Profiler::FPS())).draw(14, Vec2{ 10, Scene::Height() - 25 }, ColorF{ 0.5 });
			}

			// ==========================================
			// 状態遷移の判定
			// ==========================================
			if (!player.isAlive()) {
				// プレイヤーのHPが0になったらゲームオーバー画面へ
				gameState = GameState::GameOver;
			}
			else if (enemies.isEmpty()) {
				// 敵をすべて倒したらゲームクリア画面へ
				gameState = GameState::GameClear;
			}
			break;
		}
		case GameState::GameOver: // ゲームオーバー画面
		{
			// 背景に薄暗いゲーム画面を描画
			{
				Graphics3D::SetCameraTransform(camera);
				Scene::SetBackground(ColorF{ 0.05, 0.0, 0.0 });
				Plane{ Vec3{0, 0, 0}, GroundSize }.draw(ColorF{ 0.2, 0.0, 0.0 });
				player.draw();
			}

			// ゲームオーバーのテキストを描画
			RectF{ 0, 0, Scene::Size() }.draw(ColorF{ 0.0, 0.0, 0.0, 0.7 });
			titleFont(U"GAME OVER").drawAt(Scene::CenterF().movedBy(0, -50), Palette::Red);
			font(U"Press [ENTER] to Title").drawAt(Scene::CenterF().movedBy(0, 50), Palette::White);

			// EnterキーまたはゲームパッドのSTARTボタンでタイトル画面に戻る
			if (KeyEnter.down() || XInput(0).buttonStart.down()) {
				gameState = GameState::Title;
			}
			break;
		}
		case GameState::GameClear: // ゲームクリア画面
		{
			// 背景に薄暗いゲーム画面を描画
			{
				Graphics3D::SetCameraTransform(camera);
				Scene::SetBackground(ColorF{ 0.0, 0.05, 0.0 });
				Plane{ Vec3{0, 0, 0}, GroundSize }.draw(ColorF{ 0.0, 0.2, 0.0 });
				player.draw();
			}

			// ゲームクリアのテキストを描画
			RectF{ 0, 0, Scene::Size() }.draw(ColorF{ 0.0, 0.0, 0.0, 0.5 });
			titleFont(U"GAME CLEAR!").drawAt(Scene::CenterF().movedBy(0, -50), Palette::Yellow);
			font(U"Press [ENTER] to Title").drawAt(Scene::CenterF().movedBy(0, 50), Palette::White);

			// EnterキーまたはゲームパッドのSTARTボタンでタイトル画面に戻る
			if (KeyEnter.down() || XInput(0).buttonStart.down()) {
				gameState = GameState::Title;
			}
			break;
		}
		}
	}
}
