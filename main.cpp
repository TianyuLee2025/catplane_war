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

// ��Ϸ����
const int WIDTH = 480;
const int HEIGHT = 600;
const int PLANE_SIZE = 40;
const int BULLET_SIZE = 16;
const int ENEMY_SIZE = 30;
const int BULLET_SPEED = 10;
const int ENEMY_SPEED_BASE = 3;
const int ENEMY_FREQ = 30;
const int PLANE_TYPES = 8; // 8�ַɻ�����


// �ɻ�����
const TCHAR* PLANE_NAMES[] = {
    _T("�ڵ�"), _T("�˸�"), _T("������"),
    _T("���˵�"), _T("���"), _T("���"),
    _T("��"), _T("С��")
};

// �����ļ�
const TCHAR* MUSIC_FILES[] = {
    _T("music1.mp3"), // �˵�����ʷ��¼����
    _T("music2.mp3"), // ��Ϸ��������
    _T("music3.mp3"), // ��Ϸ��������
    _T("music4.mp3")  // �ӵ�������Ч
};

// ��Ϸģʽ
enum GameMode { EASY, HARD };
GameMode currentMode = EASY;

// ��ť�ṹ��
struct Button {
    int x, y, width, height;
    const TCHAR* text;
    bool hover;
};

// ��ʷ��¼�ṹ��
struct Record {
    TCHAR planeName[20];
    int score;
    GameMode mode;
    time_t time;
};

// ͼƬ��Դ
IMAGE imgMenuBackground;    // ͨ�ñ���
IMAGE imgEasyBackground;  // ��ģʽ����
IMAGE imgHardBackground;  // ����ģʽ����
IMAGE imgPlane[PLANE_TYPES];  // 8�ַɻ�ͼƬ
IMAGE imgBullet;             // �ӵ�
IMAGE imgBackground;         // ����

// ������Դ����
void loadResources() {
    // ����8�ַɻ�ͼƬ
    for (int i = 0; i < PLANE_TYPES; i++) {
        TCHAR filename[20];
        _stprintf(filename, _T("plane%d.png"), i + 1);
        loadimage(&imgPlane[i], filename, PLANE_SIZE, PLANE_SIZE);
    }

    //����ģʽ������Ƭ
    loadimage(&imgMenuBackground, _T("background1.jpg"), WIDTH, HEIGHT);
    loadimage(&imgEasyBackground, _T("gamebackground1.jpg"), WIDTH, HEIGHT);
    loadimage(&imgHardBackground, _T("gamebackground2.jpg"), WIDTH, HEIGHT);

    // �����ӵ�ͼƬ
    loadimage(&imgBullet, _T("round.png"), BULLET_SIZE, BULLET_SIZE);

    // ���ر���ͼƬ
    loadimage(&imgBackground, _T("background2.jpg"), WIDTH, HEIGHT);
}

// �ɻ���
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

// �ӵ���
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
        // �����ӵ�
        putimage(x - BULLET_SIZE / 2, y - BULLET_SIZE / 2,
            BULLET_SIZE, BULLET_SIZE,
            img, 0, 0, SRCCOPY);
    }
};

// �л���
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

// ��Ϸ��
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
    bool inRankingMenu;  // �Ƿ������а�ѡ��˵�
    bool inEasyRanking;  // �Ƿ��ڼ�ģʽ���а�
    bool inHardRanking;  // �Ƿ�������ģʽ���а�
    int selectedPlaneIndex;
    Button easyBtn, hardBtn, exitBtn, restartBtn, menuBtn, recordsBtn, rulesBtn, rankingBtn;
    Button easyRankBtn, hardRankBtn; // ���а�ģʽѡ��ť
    Button planeBtns[PLANE_TYPES];
    vector<Record> gameRecords;
    int currentMusic;
    bool musicPlaying;
    int soundEffectCounter;

    // ���ű�������
    void playBackgroundMusic(int musicType) {
        if (currentMusic == musicType && musicPlaying) return;

        // ֹͣ��ǰ����
        if (musicPlaying) {
            mciSendString(_T("stop bg_music"), NULL, 0, NULL);
            mciSendString(_T("close bg_music"), NULL, 0, NULL);
            musicPlaying = false;
        }

        // ����������
        if (musicType >= 0 && musicType < 3) {
            TCHAR cmd[256];
            _stprintf(cmd, _T("open \"%s\" alias bg_music"), MUSIC_FILES[musicType]);
            if (mciSendString(cmd, NULL, 0, NULL) != 0) {
                return; // �����ļ�����ʧ��
            }

            mciSendString(_T("play bg_music repeat"), NULL, 0, NULL);

            currentMusic = musicType;
            musicPlaying = true;
        }
    }

    // ������Ч
    void playSoundEffect(int musicType) {
        if (musicType == 3) { // �ӵ���Ч
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

    // ֹͣ��������
    void stopMusic() {
        if (musicPlaying) {
            mciSendString(_T("stop bg_music"), NULL, 0, NULL);
            mciSendString(_T("close bg_music"), NULL, 0, NULL);
            musicPlaying = false;
            currentMusic = -1;
        }
        // ֹͣ�������ڲ��ŵ���Ч
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

        // ��ʼ�����˵���ť
        easyBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 100, 200, 40, _T("��ģʽ"), false };
        hardBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 40, _T("����ģʽ"), false };
        recordsBtn = { WIDTH / 2 - 100, HEIGHT / 2, 200, 40, _T("��ʷ��¼"), false };
        rulesBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("��Ϸ����"), false };
        rankingBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 100, 200, 40, _T("���а�"), false };
        exitBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("�˳���Ϸ"), false };

        // ��ʼ����Ϸ������ť
        restartBtn = { WIDTH / 2 - 100, HEIGHT / 2, 200, 40, _T("���¿�ʼ"), false };
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("���ز˵�"), false };

        // ��ʼ�����а�ģʽѡ��ť
        easyRankBtn = { WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 40, _T("��ģʽ���а�"), false };
        hardRankBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 50, 200, 40, _T("����ģʽ���а�"), false };

        // ��ʼ���ɻ�ѡ��ť
        for (int i = 0; i < PLANE_TYPES; i++) {
            int row = i / 4;
            int col = i % 4;
            planeBtns[i] = { WIDTH / 2 - 200 + col * 100, HEIGHT / 2 - 100 + row * 100,
                            80, 100, _T(""), false };
        }

        // ������ʷ��¼
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

        // �Ѷȵ���
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

            // ���ѡ��л����ͣ����������ѡ��ķɻ���
            int enemyType;
            do {
                enemyType = rand() % PLANE_TYPES;
            } while (enemyType == selectedPlaneIndex);

            int speed = ENEMY_SPEED_BASE;
            if (currentMode == HARD) {
                speed += gameLevel / 2; // �Ѷ�Խ�ߵл�Խ��
            }

            enemies.emplace_back(x, -ENEMY_SIZE / 2, &imgPlane[enemyType], speed);
        }

        checkCollisions();
    }

    // ���ư�ť
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

    // �������Ƿ��ڰ�ť��
    bool checkButtonHover(Button& btn, int mouseX, int mouseY) {
        btn.hover = (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height);
        return btn.hover;
    }

    // ����ť���
    bool checkButtonClick(Button& btn, MOUSEMSG msg) {
        if (msg.uMsg == WM_LBUTTONDOWN &&
            msg.x >= btn.x && msg.x <= btn.x + btn.width &&
            msg.y >= btn.y && msg.y <= btn.y + btn.height) {
            return true;
        }
        return false;
    }

    // ������ʷ��¼
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

    // ������ʷ��¼
    void saveRecords() {
        ofstream file("records.dat", ios::binary);
        for (const auto& record : gameRecords) {
            file.write((const char*)&record, sizeof(Record));
        }
        file.close();
    }

    // ����¼�¼
    void addRecord() {
        Record newRecord;
        _tcscpy(newRecord.planeName, PLANE_NAMES[selectedPlaneIndex]);
        newRecord.score = score;
        newRecord.mode = currentMode;
        newRecord.time = time(nullptr);

        gameRecords.push_back(newRecord);
        saveRecords();
    }

    // ���ƹ������
    void drawRules() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 80, HEIGHT / 2 - 200, _T("��Ϸ����"));

        // ��������
        settextstyle(20, 0, _T("Microsoft YaHei"));

        // �����ı�����
        const TCHAR* rules[] = {
            _T("1. ʹ������ƶ���������"),
            _T("2. �������������èצ����"),
            _T("3. ����з��������10��"),
            _T("4. ������з�������ײ"),
            _T("5. ��ģʽ: �з������ٶȽ���"),
            _T("6. ����ģʽ: �з������ٶ���ȼ�����"),
            _T("7. ѡ����ϲ����������ʼ��Ϸ"),
            _T("8. ����߷ֵ������а�!")
        };

        // ���ƹ����ı�
        int startY = HEIGHT / 2 - 150;
        for (int i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
            outtextxy(WIDTH / 2 - 200, startY + i * 30, rules[i]);
        }

        // ���ذ�ť
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("���ز˵�"), false };
        drawButton(menuBtn);
    }

    // �������а�ѡ��˵�
    void drawRankingMenu() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 50, HEIGHT / 2 - 180, _T("���а�"));

        // ����ģʽѡ��ť
        drawButton(easyRankBtn);
        drawButton(hardRankBtn);

        // ���Ʒ��ذ�ť
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("���ز˵�"), false };
        drawButton(menuBtn);
    }

    // ���Ƽ�ģʽ���а�
    void drawEasyRanking() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("��ģʽ���а�"));

        // ���Ʒ��ذ�ť
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("���ز˵�"), false };
        drawButton(menuBtn);

        // ɸѡ��ģʽ��¼
        vector<Record> easyRecords;
        for (const auto& record : gameRecords) {
            if (record.mode == EASY) {
                easyRecords.push_back(record);
            }
        }

        // �������Ӹߵ�������
        sort(easyRecords.begin(), easyRecords.end(),
            [](const Record& a, const Record& b) { return a.score > b.score; });

        // ֻ��ʾǰ5����¼
        int showCount = min(5, (int)easyRecords.size());
        int startY = HEIGHT / 2 - 120;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = easyRecords[i];

            // ��ʽ��ʱ��
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // ��ʽ����¼��Ϣ
            TCHAR rankStr[100];
            _stprintf(rankStr, _T("%d. %s %d�� %s"),
                i + 1, record.planeName, record.score, timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 180, startY + i * 30, rankStr);
        }

        // ���û�м�¼
        if (easyRecords.empty()) {
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("���޼�¼"));
        }
    }

    // ��������ģʽ���а�
    void drawHardRanking() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("����ģʽ���а�"));

        // ���Ʒ��ذ�ť
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("���ز˵�"), false };
        drawButton(menuBtn);

        // ɸѡ����ģʽ��¼
        vector<Record> hardRecords;
        for (const auto& record : gameRecords) {
            if (record.mode == HARD) {
                hardRecords.push_back(record);
            }
        }

        // �������Ӹߵ�������
        sort(hardRecords.begin(), hardRecords.end(),
            [](const Record& a, const Record& b) { return a.score > b.score; });

        // ֻ��ʾǰ5����¼
        int showCount = min(5, (int)hardRecords.size());
        int startY = HEIGHT / 2 - 120;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = hardRecords[i];

            // ��ʽ��ʱ��
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // ��ʽ����¼��Ϣ
            TCHAR rankStr[100];
            _stprintf(rankStr, _T("%d. %s %d�� %s"),
                i + 1, record.planeName, record.score, timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 180, startY + i * 30, rankStr);
        }

        // ���û�м�¼
        if (hardRecords.empty()) {
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("���޼�¼"));
        }
    }

    void draw() {
        cleardevice();

        // ���Ʊ���
        if (!isGameOver && !inMenu && !inPlaneSelection && !inRecords && !inRules && !inRankingMenu) {
            // ��Ϸ�����У�����ģʽѡ�񱳾�
            if (currentMode == EASY) {
                putimage(0, 0, &imgEasyBackground);
            }
            else {
                putimage(0, 0, &imgHardBackground);
            }
        }
        else {
            // �����������ʹ�ò˵�����
            putimage(0, 0, &imgMenuBackground);
        }

        // ������Ϸ״̬���Ŷ�Ӧ�ı�������
        if (inMenu || inRecords || inRules || inRankingMenu || inEasyRanking || inHardRanking) {
            playBackgroundMusic(0); // �˵�����ʷ��¼����
        }
        else if (!isGameOver && !inPlaneSelection) {
            playBackgroundMusic(1); // ��Ϸ��������
        }
        else if (isGameOver) {
            playBackgroundMusic(2); // ��Ϸ��������
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

        // ���Ʒ����͵ȼ�
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(20, 0, _T("Microsoft YaHei"));
        TCHAR scoreStr[50];
        _stprintf(scoreStr, _T("����: %d  �ȼ�: %d"), score, gameLevel);
        outtextxy(10, 10, scoreStr);

        if (isGameOver) {
            drawGameOver();
        }
    }

    void drawMenu() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 50, HEIGHT / 2 - 150, _T("������ս"));

        // ���Ʋ˵���ť
        drawButton(easyBtn);
        drawButton(hardBtn);
        drawButton(recordsBtn);
        drawButton(rulesBtn);
        drawButton(rankingBtn);
        drawButton(exitBtn);
    }

    void drawPlaneSelection() {

        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(30, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 70, HEIGHT / 2 - 150, _T("ѡ���������"));

        // ���Ʒɻ�ѡ��ť������
        for (int i = 0; i < PLANE_TYPES; i++) {
            drawButton(planeBtns[i]);
            // ���Ʒɻ�ͼƬ
            putimage(planeBtns[i].x + (planeBtns[i].width - PLANE_SIZE) / 2,
                planeBtns[i].y + 10, &imgPlane[i]);

            // ���Ʒɻ�����
            settextcolor(RGB(219, 112, 147));
            setbkmode(TRANSPARENT);
            settextstyle(16, 0, _T("Microsoft YaHei"));
            int textWidth = textwidth(PLANE_NAMES[i]);
            outtextxy(planeBtns[i].x + (planeBtns[i].width - textWidth) / 2,
                planeBtns[i].y + 60, PLANE_NAMES[i]);
        }
    }

    void drawRecords() {
        // ���Ʊ���
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 180, _T("��ʷ��¼"));

        // ���Ʒ��ذ�ť
        menuBtn = { WIDTH / 2 - 100, HEIGHT / 2 + 150, 200, 40, _T("���ز˵�"), false };
        drawButton(menuBtn);

        // ���Ƽ�¼�б�
        if (gameRecords.empty()) {
            settextstyle(24, 0, _T("Microsoft YaHei"));
            outtextxy(WIDTH / 2 - 80, HEIGHT / 2, _T("���޼�¼"));
            return;
        }

        // ��ʱ����µ����������µļ�¼��ǰ��
        sort(gameRecords.begin(), gameRecords.end(),
            [](const Record& a, const Record& b) { return a.time > b.time; });

        // ֻ��ʾ���µ�8����¼
        int showCount = min(8, (int)gameRecords.size());
        int startY = HEIGHT / 2 - 140;

        settextstyle(20, 0, _T("Microsoft YaHei"));
        for (int i = 0; i < showCount; i++) {
            const Record& record = gameRecords[i];

            // ��ʽ��ʱ��
            struct tm timeinfo;
            localtime_s(&timeinfo, &record.time);
            TCHAR timeStr[20];
            _tcsftime(timeStr, 20, _T("%Y-%m-%d"), &timeinfo);

            // ��ʽ����¼��Ϣ
            TCHAR recordStr[100];
            _stprintf(recordStr, _T("%s %s %d�� %s"),
                record.mode == EASY ? _T("��") : _T("����"),
                record.planeName,
                record.score,
                timeStr);

            settextcolor(RGB(219, 112, 147));
            outtextxy(WIDTH / 2 - 200, startY + i * 35, recordStr);
        }
    }

    void drawGameOver() {

        // ������Ϸ��������
        settextcolor(RGB(219, 112, 147));
        setbkmode(TRANSPARENT);
        settextstyle(36, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 60, HEIGHT / 2 - 80, _T("��Ϸ����"));

        // �������շ���
        TCHAR scoreStr[50];
        _stprintf(scoreStr, _T("���շ���: %d"), score);
        settextstyle(24, 0, _T("Microsoft YaHei"));
        outtextxy(WIDTH / 2 - 60, HEIGHT / 2 - 30, scoreStr);

        // ������Ϸ������ť
        drawButton(restartBtn);
        drawButton(menuBtn);
    }

    void fire() {
        if (!isGameOver && !inMenu && !inPlaneSelection && !inRecords &&
            !inRules && !inRankingMenu && !inEasyRanking && !inHardRanking) {
            bullets.emplace_back(player.x, player.y - PLANE_SIZE / 2, &imgBullet);
            playSoundEffect(3); // �����ӵ���Ч
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
                addRecord(); // ��Ϸ����ʱ�����¼
            }
        }
    }

    void handleInput() {
        MOUSEMSG msg;
        while (MouseHit()) {
            msg = GetMouseMsg();

            if (inMenu) {
                // �˵�����������
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
                // �ɻ�ѡ�����
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
                // ��ʷ��¼����
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inRecords = false;
                    inMenu = true;
                }
            }
            else if (inRules) {
                // �������
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inRules = false;
                    inMenu = true;
                }
            }
            else if (inRankingMenu) {
                // ���а�ѡ��˵�
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
                // ���а����
                checkButtonHover(menuBtn, msg.x, msg.y);

                if (msg.uMsg == WM_LBUTTONDOWN && checkButtonClick(menuBtn, msg)) {
                    inEasyRanking = false;
                    inHardRanking = false;
                    inRankingMenu = true;
                }
            }
            else if (isGameOver) {
                // ��Ϸ��������������
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
                // ��Ϸ������������
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
            Sleep(30); // ����֡��
        }
    }
};

int main() {

    // �����������
    srand(static_cast<unsigned>(time(nullptr)));
    // ��ʼ��ͼ�δ���
    initgraph(WIDTH, HEIGHT, EW_SHOWCONSOLE | EW_DBLCLKS);
    SetWindowText(GetHWnd(), _T("������ս"));

    // ����˫����
    BeginBatchDraw();
    setbkcolor(WHITE);
    cleardevice();

    // ������Դ
    loadResources();

    // ��ʼ����Ϸ
    Game game;

    // ��Ϸ��ѭ��
    DWORD lastTime = GetTickCount();
    while (true) {
        // ����֡��
        DWORD now = GetTickCount();
        if (now - lastTime < 30) {  // Լ33FPS
            Sleep(1);
            continue;
        }
        lastTime = now;

        // ��������
        game.handleInput();

        // ������Ϸ״̬
        game.update();

        // ����������
        cleardevice();
        game.draw();

        // ˢ�»�����
        FlushBatchDraw();
    }

    // ��Ϸ��������
    EndBatchDraw();
    closegraph();
    return 0;
}