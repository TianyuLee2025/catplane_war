#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <graphics.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string>
#include <vector>
#pragma comment(lib, "winmm.lib")
using namespace std;

// 游戏常量
const int WIDTH = 480;
const int HEIGHT = 600;
const int PLANE_SIZE = 40;
const int BULLET_SIZE = 16;
const int ENEMY_SIZE = 30;
const int BULLET_SPEED = 10;
const int ENEMY_SPEED_BASE = 3;
const int ENEMY_FREQ = 30;
const int PLANE_TYPES = 8; // 8种飞机类型


// 飞机名称
const TCHAR* PLANE_NAMES[] = {
    _T("黑蛋"), _T("八嘎"), _T("奥利奥"),
    _T("黑怂蛋"), _T("嗷呜"), _T("咖喱"),
    _T("窦娥"), _T("小猪")
};

// 音乐文件
const TCHAR* MUSIC_FILES[] = {
    _T("music1.mp3"), // 菜单和历史记录音乐
    _T("music2.mp3"), // 游戏过程音乐
    _T("music3.mp3"), // 游戏结束音乐
    _T("music4.mp3")  // 子弹发射音效
};

// 游戏模式
enum GameMode { EASY, HARD };
GameMode currentMode = EASY;

// 按钮结构体
struct Button {
    int x, y, width, height;
    const TCHAR* text;
    bool hover;
};

// 历史记录结构体
struct Record {
    TCHAR planeName[20];
    int score;
    GameMode mode;
    time_t time;
};

// 图片资源
IMAGE imgMenuBackground;    // 通用背景
IMAGE imgEasyBackground;  // 简单模式背景
IMAGE imgHardBackground;  // 困难模式背景
IMAGE imgPlane[PLANE_TYPES];  // 8种飞机图片
IMAGE imgBullet;             // 子弹
IMAGE imgBackground;         // 背景

// 加载资源函数
void loadResources() {
    // 加载8种飞机图片
    for (int i = 0; i < PLANE_TYPES; i++) {
        TCHAR filename[20];
        _stprintf(filename, _T("plane%d.png"), i + 1);
        loadimage(&imgPlane[i], filename, PLANE_SIZE, PLANE_SIZE);
    }

    //加载模式背景照片
    loadimage(&imgMenuBackground, _T("background1.jpg"), WIDTH, HEIGHT);
    loadimage(&imgEasyBackground, _T("gamebackground1.jpg"), WIDTH, HEIGHT);
    loadimage(&imgHardBackground, _T("gamebackground2.jpg"), WIDTH, HEIGHT);

    // 加载子弹图片
    loadimage(&imgBullet, _T("round.png"), BULLET_SIZE, BULLET_SIZE);

    // 加载背景图片
    loadimage(&imgBackground, _T("background2.jpg"), WIDTH, HEIGHT);
}

// 飞机类
class Plane {
public:
    int x, y;
    int speed;
    IMAGE* img;

    Plane(int _x, int _y, int _speed, IMAGE* _img)
        : x(_x), y(_y), speed(_speed), img(_img) {}

    void draw() {
        putimage(x - PLANE_SIZE / 2, y - PLANE_SIZE / 2, img);
    }

    void move(int dx) {
        x += dx * speed;
        if (x < PLANE_SIZE / 2) x = PLANE_SIZE / 2;
        if (x > WIDTH - PLANE_SIZE / 2) x = WIDTH - PLANE_SIZE / 2;
    }
};

// 子弹类
class Bullet {
public:
    int x, y;
    bool active;
    IMAGE* img;

    Bullet(int _x, int _y, IMAGE* _img)
        : x(_x), y(_y), active(true), img(_img) {}

    void update() {
        y -= BULLET_SPEED;
        if (y < 0) active = false;
    }

    void draw() {
        // 绘制子弹
        putimage(x - BULLET_SIZE / 2, y - BULLET_SIZE / 2,
            BULLET_SIZE, BULLET_SIZE,
            img, 0, 0, SRCCOPY);
    }
};

// 敌机类
class Enemy {
public:
    int x, y;
    bool active;
    IMAGE* img;
    int speed;

    Enemy(int _x, int _y, IMAGE* _img, int _speed)
        : x(_x), y(_y), active(true), img(_img), speed(_speed) {}

    void update() {
        y += speed;
        if (y > HEIGHT) active = false;
    }

    void draw() {
        putimage(x - ENEMY_SIZE / 2, y - ENEMY_SIZE / 2, img);
    }
};

// 游戏类
class Game {
private:
    Plane player;
    vector<Bullet> bullets;
    vector<Enemy> enemies;
    int score;
    bool isGameOver;
    int enemyCounter;
    int gameLevel;
    bool inMenu;
    bool inPlaneSelection;
    bool inRecords;
    bool inRules;
    bool inRankingMenu;  // 是否在排行榜选择菜单
    bool inEasyRanking;  // 是否在简单模式排行榜
    bool inHardRanking;  // 是否在困难模式排行榜
    int selectedPlaneIndex;
    Button easyBtn, hardBtn, exitBtn, restartBtn, menuBtn, recordsBtn, rulesBtn, rankingBtn;
    Button easyRankBtn, hardRankBtn; // 排行榜模式选择按钮
    Button planeBtns[PLANE_TYPES];
    vector<Record> gameRecords;
    int currentMusic;
    bool musicPlaying;
    int soundEffectCounter;

    // 播放背景音乐
    void playBackgroundMusic(int musicType) {
        if (currentMusic == musicType && musicPlaying) return;

        // 停止当前音乐
        if (musicPlaying) {
            mciSendString(_T("stop bg_music"), NULL, 0, NULL);
            mciSendString(_T("close bg_music"), NULL, 0, NULL);
            musicPlaying = false;
        }

        // 播放新音乐
        if (musicType >= 0 && musicType < 3) {
            TCHAR cmd[256];
            _stprintf(cmd, _T("open \"%s\" alias bg_music"), MUSIC_FILES[musicType]);
            if (mciSendString(cmd, NULL, 0, NULL) != 0) {
                return; // 音乐文件加载失败
            }

            mciSendString(_T("play bg_music repeat"), NULL, 0, NULL);

            currentMusic = musicType;
            musicPlaying = true;
        }
    }

    // 播放音效
    void playSoundEffect(int musicType) {
        if (musicType == 3) { // 子弹音效
            TCHAR cmd[256];
            TCHAR alias[20];
            _stprintf(alias, _T("sfx%d"), soundEffectCounter++);

            _stprintf(cmd, _T("open \"%s\" alias %s"), MUSIC_FILES[musicType], alias);
            if (mciSendString(cmd, NULL, 0, NULL) == 0) {
                _stprintf(cmd, _T("play %s"), alias);
                mciSendString(cmd, NULL, 0, NULL);
            }
        }
    }

    // 停止所有音乐
    void stopMusic() {
        if (musicPlaying) {
            mciSendString(_T("stop bg_music"), NULL, 0, NULL);
            mciSendString(_T("close bg_music"), NULL, 0, NULL);
            musicPlaying = false;
            currentMusic = -1;
        }
        // 停止可能正在播放的音效
        mciSendString(_T("close sfx"), NULL, 0, NULL);
    }

public:
    Game() : player(WIDTH / 2, HEIGHT - 50, 8, &imgPlane[0]),
        score(0), isGameOver(false), enemyCounter(0),
        gameLevel(1), inMenu(true), inPlaneSelection(false),
        inRecords(false), inRules(false), inRankingMenu(false),
        inEasyRanking(false), inHardRanking(false),
        selectedPlaneIndex(0), currentMusic(-1), musicPlaying(false),
        soundEffectCounter(0) {

        // 初始化主菜单按钮
        easyBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 100, 200, 40, _T("简单模式"), false };
        hardBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 40, _T("困难模式"), false };
        recordsBtn = { WIDTH / 2 - 100, HEIGHT / 2, 200, 40, _T("历史记录"), false };
        rulesBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("游戏规则"), false };
        rankingBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 100, 200, 40, _T("排行榜"), false };
        exitBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("退出游戏"), false };

        // 初始化游戏结束按钮
        restartBtn = { WIDTH / 2 - 100, HEIGHT / 2, 200, 40, _T("重新开始"), false };
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("返回菜单"), false };

        // 初始化排行榜模式选择按钮
        easyRankBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 40, _T("简单模式排行榜"), false };
        hardRankBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("困难模式排行榜"), false };

        // 初始化飞机选择按钮
        for (int i = 0; i < PLANE_TYPES; i++) {
            int row = i / 4;
            int col = i % 4;
            planeBtns[i] = { WIDTH / 2 - 200 + col * 100, HEIGHT / 2 - 100 + row * 100,
                            80, 100, _T(""), false };
        }

        // 加载历史记录
        loadRecords();
    }

    ~Game() {
        stopMusic();
    }

    void init() {
        bullets.clear();
        enemies.clear();
        score = 0;
        isGameOver = false;
        enemyCounter = 0;
        gameLevel = 1;
        player.x = WIDTH / 2;
        player.y = HEIGHT - 50;
    }

    void update() {
        if (isGameOver || inMenu || inPlaneSelection || inRecords || inRules ||
            inRankingMenu || inEasyRanking || inHardRanking) return;

        // 难度递增
        if (score > gameLevel * 100) {
            gameLevel++;
        }

        for (auto& bullet : bullets) bullet.update();
        for (auto& enemy : enemies) enemy.update();

        bullets.erase(remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }), bullets.end());
        enemies.erase(remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.active; }), enemies.end());

        if (++enemyCounter >= ENEMY_FREQ) {
            enemyCounter = 0;
            int x = rand() % (WIDTH - ENEMY_SIZE) + ENEMY_SIZE / 2;

            // 随机选择敌机类型（不能是玩家选择的飞机）
            int enemyType;
            do {
                enemyType = rand() % PLANE_TYPES;
            } while (enemyType == selectedPlaneIndex);

            int speed = ENEMY_SPEED_BASE;
            if (currentMode == HARD) {
                speed += gameLevel / 2; // 难度越高敌机越快
            }

            enemies.emplace_back(x, -ENEMY_SIZE / 2, &imgPlane[enemyType], speed);
        }

        checkCollisions();
    }

    // 绘制按钮
    void drawButton(const Button& btn) {
        setfillcolor(btn.hover ? RGB(255, 214, 228) : RGB(255, 240, 245));
        setlinecolor(RGB(219, 112, 147));
        fillrectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);
        rectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);

        if (btn.text[0] != '\0') {
            settextcolor(RGB(219, 112, 147));
            setbkmode(TRANSPARENT);
            settextstyle(20, 0, _T("Microsoft YaHei"));
            int textWidth = textwidth(btn.text);
            int textHeight = textheight(btn.text);
            outtextxy(btn.x + (btn.width - textWidth) / 2, btn.y + (btn.height - textHeight) / 2, btn.text);
        }
    }

    // 检查鼠标是否在按钮上
    bool checkButtonHover(Button& btn, int mouseX, int mouseY) {
        btn.hover = (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height);
        return btn.hover;
    }

    // 处理按钮点击
    bool checkButtonClick(Button& btn, MOUSEMSG msg) {
        if (msg.uMsg == WM_LBUTTONDOWN &&
            msg.x >= btn.x && msg.x <= btn.x + btn.width &&
            msg.y >= btn.y && msg.y <= btn.y + btn.height) {
            return true;
        }
        return false;
    }

    // 加载历史记录
    void loadRecords() {
        gameRecords.clear();
        ifstream file("records.dat", ios::binary);
        if (file) {
            while (file.peek() != EOF) {
                Record record;
                file.read((char*)&record, sizeof(Record));
                gameRecords.push_back(record);
            }
            file.close();
        }
    }

    // 保存历史记录
    void saveRecords() {
        ofstream file("records.dat", ios::binary);
        for (const auto& record : gameRecords) {
            file.write((const char*)&record, sizeof(Record));
        }
        file.close();
    }

    // 添加新记录
    void addRecord() {
        Record newRecord;
        _tcscpy(newRecord.planeName, PLANE_NAMES[selectedPlaneIndex]);
        newRecord.score = score;
        newRecord.mode = currentMode;
        newRecord.time = time(nullptr);

        gameRecords.push_back(newRecord);
        saveRecords();
    }

    // 绘制规则界面
    void drawRules() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 80, HEIGHT / 2 - 200, _T("游戏规则"));

        // 规则内容
        settextstyle(20, 0, _T("Microsoft YaHei"));

        // 规则文本数组
        const TCHAR* rules[] = {
            _T("1. 使用鼠标移动控制喵机"),
            _T("2. 点击鼠标左键发射猫爪攻击"),
            _T("3. 击落敌方喵机获得10分"),
            _T("4. 避免与敌方喵机相撞"),
            _T("5. 简单模式: 敌方喵机速度较慢"),
            _T("6. 困难模式: 敌方喵机速度随等级提升"),
            _T("7. 选择你喜欢的喵机开始游戏"),
            _T("8. 创造高分登上排行榜!")
        };

        // 绘制规则文本
        int startY = HEIGHT / 2 - 150;
        for (int i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
            outtextxy(WIDTH / 2 - 200, startY + i * 30, rules[i]);
        }

        // 返回按钮
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("返回菜单"), false };
        drawButton(menuBtn);
    }

    // 绘制排行榜选择菜单
    void drawRankingMenu() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 50, HEIGHT / 2 - 180, _T("排行榜"));

        // 绘制模式选择按钮
        drawButton(easyRankBtn);
        drawButton(hardRankBtn);

        // 绘制返回按钮
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("返回菜单"), false };
        drawButton(menuBtn);
    }

    // 绘制简单模式排行榜
    void drawEasyRanking() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("简单模式排行榜"));

        // 绘制返回按钮
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("返回菜单"), false };
        drawButton(menuBtn);

        // 筛选简单模式记录
        vector<Record> easyRecords;
        for (const auto& record : gameRecords) {
            if (record.mode == EASY) {
                easyRecords.push_back(record);
            }
        }

        // 按分数从高到低排序
        sort(easyRecords.begin(), easyRecords.end(),
            [](const Record& a, const Record& b) { return a.score > b.score; });

        // 只显示前5条记录
        int showCount = min(5, (int)easyRecords.size());
        int startY = HEIGHT / 2 - 120;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = easyRecords[i];

            // 格式化时间
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // 格式化记录信息
            TCHAR rankStr[100];
            _stprintf(rankStr, _T("%d. %s %d分 %s"),
                i + 1, record.planeName, record.score, timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 180, startY + i * 30, rankStr);
        }

        // 如果没有记录
        if (easyRecords.empty()) {
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("暂无记录"));
        }
    }

    // 绘制困难模式排行榜
    void drawHardRanking() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("困难模式排行榜"));

        // 绘制返回按钮
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("返回菜单"), false };
        drawButton(menuBtn);

        // 筛选困难模式记录
        vector<Record> hardRecords;
        for (const auto& record : gameRecords) {
            if (record.mode == HARD) {
                hardRecords.push_back(record);
            }
        }

        // 按分数从高到低排序
        sort(hardRecords.begin(), hardRecords.end(),
            [](const Record& a, const Record& b) { return a.score > b.score; });

        // 只显示前5条记录
        int showCount = min(5, (int)hardRecords.size());
        int startY = HEIGHT / 2 - 120;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = hardRecords[i];

            // 格式化时间
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // 格式化记录信息
            TCHAR rankStr[100];
            _stprintf(rankStr, _T("%d. %s %d分 %s"),
                i + 1, record.planeName, record.score, timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 180, startY + i * 30, rankStr);
        }

        // 如果没有记录
        if (hardRecords.empty()) {
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("暂无记录"));
        }
    }

    void draw() {
        cleardevice();

        // 绘制背景
        if (!isGameOver && !inMenu && !inPlaneSelection && !inRecords && !inRules && !inRankingMenu) {
            // 游戏进行中，根据模式选择背景
            if (currentMode == EASY) {
                putimage(0, 0, &imgEasyBackground);
            }
            else {
                putimage(0, 0, &imgHardBackground);
            }
        }
        else {
            // 其他所有情况使用菜单背景
            putimage(0, 0, &imgMenuBackground);
        }

        // 根据游戏状态播放对应的背景音乐
        if (inMenu || inRecords || inRules || inRankingMenu || inEasyRanking || inHardRanking) {
            playBackgroundMusic(0); // 菜单和历史记录音乐
        }
        else if (!isGameOver && !inPlaneSelection) {
            playBackgroundMusic(1); // 游戏过程音乐
        }
        else if (isGameOver) {
            playBackgroundMusic(2); // 游戏结束音乐
        }

        if (inMenu) {
            drawMenu();
            return;
        }

        if (inPlaneSelection) {
            drawPlaneSelection();
            return;
        }

        if (inRecords) {
            drawRecords();
            return;
        }

        if (inRules) {
            drawRules();
            return;
        }

        if (inRankingMenu) {
            drawRankingMenu();
            return;
        }

        if (inEasyRanking) {
            drawEasyRanking();
            return;
        }

        if (inHardRanking) {
            drawHardRanking();
            return;
        }

        player.draw();
        for (auto& bullet : bullets) bullet.draw();
        for (auto& enemy : enemies) enemy.draw();

        // 绘制分数和等级
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(20, 0, _T("Microsoft YaHei"));
        TCHAR scoreStr[50];
        _stprintf(scoreStr, _T("分数: %d  等级: %d"), score, gameLevel);
        outtextxy(10, 10, scoreStr);

        if (isGameOver) {
            drawGameOver();
        }
    }

    void drawMenu() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 50, HEIGHT / 2 - 150, _T("喵机大战"));

        // 绘制菜单按钮
        drawButton(easyBtn);
        drawButton(hardBtn);
        drawButton(recordsBtn);
        drawButton(rulesBtn);
        drawButton(rankingBtn);
        drawButton(exitBtn);
    }

    void drawPlaneSelection() {

        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(30, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 70, HEIGHT / 2 - 150, _T("选择你的喵机"));

        // 绘制飞机选择按钮和名称
        for (int i = 0; i < PLANE_TYPES; i++) {
            drawButton(planeBtns[i]);
            // 绘制飞机图片
            putimage(planeBtns[i].x + (planeBtns[i].width - PLANE_SIZE) / 2,
                planeBtns[i].y + 10, &imgPlane[i]);

            // 绘制飞机名称
            settextcolor(RGB(219, 112, 147));
            setbkmode(TRANSPARENT);
            settextstyle(16, 0, _T("Microsoft YaHei"));
            int textWidth = textwidth(PLANE_NAMES[i]);
            outtextxy(planeBtns[i].x + (planeBtns[i].width - textWidth) / 2,
                planeBtns[i].y + 60, PLANE_NAMES[i]);
        }
    }

    void drawRecords() {
        // 绘制标题
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("历史记录"));

        // 绘制返回按钮
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("返回菜单"), false };
        drawButton(menuBtn);

        // 绘制记录列表
        if (gameRecords.empty()) {
            settextstyle(24, 0, _T("Microsoft YaHei"));
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("暂无记录"));
            return;
        }

        // 按时间从新到旧排序（最新的记录在前）
        sort(gameRecords.begin(), gameRecords.end(),
            [](const Record& a, const Record& b) { return a.time > b.time; });

        // 只显示最新的8条记录
        int showCount = min(8, (int)gameRecords.size());
        int startY = HEIGHT / 2 - 140;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = gameRecords[i];

            // 格式化时间
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // 格式化记录信息
            TCHAR recordStr[100];
            _stprintf(recordStr, _T("%s %s %d分 %s"),
                record.mode == EASY ? _T("简单") : _T("困难"),
                record.planeName,
                record.score,
                timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 200, startY + i * 35, recordStr);
        }
    }

    void drawGameOver() {

        // 绘制游戏结束文字
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 60, HEIGHT / 2 - 80, _T("游戏结束"));

        // 绘制最终分数
        TCHAR scoreStr[50];
        _stprintf(scoreStr, _T("最终分数: %d"), score);
        settextstyle(24, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 60, HEIGHT / 2 - 30, scoreStr);

        // 绘制游戏结束按钮
        drawButton(restartBtn);
        drawButton(menuBtn);
    }

    void fire() {
        if (!isGameOver && !inMenu && !inPlaneSelection && !inRecords &&
            !inRules && !inRankingMenu && !inEasyRanking && !inHardRanking) {
            bullets.emplace_back(player.x, player.y - PLANE_SIZE / 2, &imgBullet);
            playSoundEffect(3); // 播放子弹音效
        }
    }

    void checkCollisions() {
        for (auto& bullet : bullets) {
            for (auto& enemy : enemies) {
                if (bullet.active && enemy.active &&
                    abs(bullet.x - enemy.x) < (BULLET_SIZE + ENEMY_SIZE) / 2 &&
                    abs(bullet.y - enemy.y) < (BULLET_SIZE + ENEMY_SIZE) / 2) {
                    bullet.active = false;
                    enemy.active = false;
                    score += 10;
                }
            }
        }

        for (auto& enemy : enemies) {
            if (enemy.active &&
                abs(player.x - enemy.x) < (PLANE_SIZE + ENEMY_SIZE) / 2 &&
                abs(player.y - enemy.y) < (PLANE_SIZE + ENEMY_SIZE) / 2) {
                isGameOver = true;
                addRecord(); // 游戏结束时保存记录
            }
        }
    }

    void handleInput() {
        MOUSEMSG msg;
        while (MouseHit()) {
            msg = GetMouseMsg();

            if (inMenu) {
                // 菜单界面鼠标控制
                checkButtonHover(easyBtn, msg.x, msg.y);
                checkButtonHover(hardBtn, msg.x, msg.y);
                checkButtonHover(recordsBtn, msg.x, msg.y);
                checkButtonHover(rulesBtn, msg.x, msg.y);
                checkButtonHover(rankingBtn, msg.x, msg.y);
                checkButtonHover(exitBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN) {
                    if (checkButtonClick(easyBtn, msg)) {
                        currentMode = EASY;
                        inMenu = false;
                        inPlaneSelection = true;
                    }
                    else if (checkButtonClick(hardBtn, msg)) {
                        currentMode = HARD;
                        inMenu = false;
                        inPlaneSelection = true;
                    }
                    else if (checkButtonClick(recordsBtn, msg)) {
                        inMenu = false;
                        inRecords = true;
                    }
                    else if (checkButtonClick(rulesBtn, msg)) {
                        inMenu = false;
                        inRules = true;
                    }
                    else if (checkButtonClick(rankingBtn, msg)) {
                        inMenu = false;
                        inRankingMenu = true;
                    }
                    else if (checkButtonClick(exitBtn, msg)) {
                        closegraph();
                        exit(0);
                    }
                }
            }
            else if (inPlaneSelection) {
                // 飞机选择界面
                for (int i = 0; i < PLANE_TYPES; i++) {
                    checkButtonHover(planeBtns[i], msg.x, msg.y);

                    if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(planeBtns[i], msg)) {
                        selectedPlaneIndex = i;
                        player.img = &imgPlane[i];
                        inPlaneSelection = false;
                        init();
                    }
                }
            }
            else if (inRecords) {
                // 历史记录界面
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inRecords = false;
                    inMenu = true;
                }
            }
            else if (inRules) {
                // 规则界面
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inRules = false;
                    inMenu = true;
                }
            }
            else if (inRankingMenu) {
                // 排行榜选择菜单
                checkButtonHover(easyRankBtn, msg.x, msg.y);
                checkButtonHover(hardRankBtn, msg.x, msg.y);
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN) {
                    if (checkButtonClick(easyRankBtn, msg)) {
                        inRankingMenu = false;
                        inEasyRanking = true;
                    }
                    else if (checkButtonClick(hardRankBtn, msg)) {
                        inRankingMenu = false;
                        inHardRanking = true;
                    }
                    else if (checkButtonClick(menuBtn, msg)) {
                        inRankingMenu = false;
                        inMenu = true;
                    }
                }
            }
            else if (inEasyRanking || inHardRanking) {
                // 排行榜界面
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inEasyRanking = false;
                    inHardRanking = false;
                    inRankingMenu = true;
                }
            }
            else if (isGameOver) {
                // 游戏结束界面鼠标控制
                checkButtonHover(restartBtn, msg.x, msg.y);
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN) {
                    if (checkButtonClick(restartBtn, msg)) {
                        init();
                    }
                    else if (checkButtonClick(menuBtn, msg)) {
                        inMenu = true;
                    }
                }
            }
            else {
                // 游戏进行中鼠标控制
                if (msg.uMsg == WM_MOUSEMOVE) {
                    player.move((msg.x - player.x) / 10);
                }
                if (msg.uMsg == WM_LBUTTONDOWN) {
                    fire();
                }
            }
        }
    }

    void run() {
        while (true) {
            handleInput();
            update();
            draw();
            Sleep(30); // 控制帧率
        }
    }
};

int main() {

    // 设置随机种子
    srand(static_cast<unsigned>(time(nullptr)));
    // 初始化图形窗口
    initgraph(WIDTH, HEIGHT, EW_SHOWCONSOLE | EW_DBLCLKS);
    SetWindowText(GetHWnd(), _T("喵机大战"));

    // 启用双缓冲
    BeginBatchDraw();
    setbkcolor(WHITE);
    cleardevice();

    // 加载资源
    loadResources();

    // 初始化游戏
    Game game;

    // 游戏主循环
    DWORD lastTime = GetTickCount();
    while (true) {
        // 控制帧率
        DWORD now = GetTickCount();
        if (now - lastTime < 30) {  // 约33FPS
            Sleep(1);
            continue;
        }
        lastTime = now;

        // 处理输入
        game.handleInput();

        // 更新游戏状态
        game.update();

        // 清屏并绘制
        cleardevice();
        game.draw();

        // 刷新缓冲区
        FlushBatchDraw();
    }

    // 游戏结束清理
    EndBatchDraw();
    closegraph();
    return 0;
}