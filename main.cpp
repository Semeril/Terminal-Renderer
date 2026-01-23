#include <bits/stdc++.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;

struct Rect
{
    double x, y, width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(double _x, double _y, double _w, double _h) : x(_x), y(_y), width(_w), height(_h) {}
    bool collides(Rect other)
    {
        return !(x + width < other.x || other.x + other.width < x || y + height < other.y || other.y + other.height < y);
    }
};

struct Color
{
    int r, g, b;

    string background()
    {
        return "\033[48;2;" + to_string(r) + ';' + to_string(g) + ';' + to_string(b) + 'm';
    }
    string foreground()
    {
        return "\033[38;2;" + to_string(r) + ';' + to_string(g) + ';' + to_string(b) + 'm';
    }

    Color(int _r, int _g, int _b)
        : r(_r), g(_g), b(_b)
    {
        if (_r > 255 || _g > 255 || _b > 255)
        {
            throw invalid_argument("Color value must be less than 256!");
        }
    }
    Color() : r(255), g(255), b(255) {}
};

struct ColoredChar
{
    char c;
    Color background_color;
    Color foreground_color;
    ColoredChar(char _c, Color bg, Color fg) : c(_c), background_color(bg), foreground_color(fg) {}
    string to_string()
    {
        return foreground_color.foreground() + background_color.background() + c + "\033[0m";
    }
};

struct Text
{
    string content;
    Color color;
    Text(string _content, Color _color) : content(_content), color(_color) {}
};

struct Screen
{
    int width, height;
    vector<vector<ColoredChar>> _list;
    void draw(Rect obj, Color color)
    {
        Rect printable = make_printable(obj);
        for (size_t dy = 0; dy < printable.height; dy++)
        {
            for (size_t dx = 0; dx < printable.width; dx++)
            {
                _list.at(printable.y + dy)[printable.x + dx] = ColoredChar(' ', color, color);
            }
        }
    }
    void draw(Text text, int x, int y)
    {
        int px = x;
        int py = y;
        for (char ch : text.content)
        {
            if (px >= width)
            {
                break;
            }
            if (ch == '\n')
            {
                px = x;
                py++;
                if (py >= height)
                {
                    break;
                }
                continue;
            }
            _list.at(py)[px] = ColoredChar(ch, Color(), text.color);
            px++;
        }
    }

    Rect make_printable(Rect obj)
    {
        obj.x = floor(obj.x * 2);
        obj.y = floor(obj.y);
        obj.width = floor(obj.width * 2);
        obj.height = floor(obj.height);
        int dw = (width) - (obj.x + obj.width);
        int dh = (height) - (obj.y + obj.height);
        if (dh < 0)
        {
            obj.height += dh;
        }
        if (obj.y < 0)
        {
            obj.height += obj.y;
        }
        if (dw < 0)
        {
            obj.width += dw;
        }
        if (obj.x < 0)
        {
            obj.width += obj.x;
        }
        obj.y = max(min(obj.y, (double)height), .0);
        obj.height = max(.0, obj.height);
        obj.x = max(min(obj.x, (double)width), .0);
        obj.width = max(.0, obj.width);
        return obj;
    }

    void clear()
    {
        for (auto &row : _list)
        {
            for (auto &ch : row)
            {
                ch = ColoredChar(' ', Color(), Color());
            }
        }
    }

    void show()
    {
        cout << "\033[H";
        string display;
        display.reserve((width + 1) * (height + 1) * (22 + 22 + 1 + 7 + 1));
        for (auto row : _list)
        {
            for (auto ch : row)
            {
                display += ch.to_string();
            }
            display += "\n";
        }
        cout << display;
    }
    Screen(int w, int h) : width(w), height(h), _list(height, vector<ColoredChar>(width, ColoredChar(' ', Color(), Color()))) {}
};

bool ispressed(int vk_code)
{
    return GetAsyncKeyState(vk_code) & 0x8000;
}

int main()
{
    cin.tie(nullptr);
    ios::sync_with_stdio(false);
    const auto FPS = 60, FRAME_TIME = 1000 / FPS, WIDTH = 120, HEIGHT = 29, RECT_WIDTH = 60;
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    Screen screen(WIDTH, HEIGHT);
    Rect ball(RECT_WIDTH / 2, HEIGHT / 2, 1, 1), paddle1(2, HEIGHT / 2 - 3, 1, 6), paddle2(RECT_WIDTH - 3, HEIGHT / 2 - 3, 1, 6);
    double ball_dx = 0.5, ball_dy = 0.5;
    double acceleration = 1.00001;
    cout << "\033[2J\033[?25l\033[0m";

    while (true)
    {
        auto frame_start = chrono::steady_clock::now();
        ball_dx *= acceleration;
        ball_dy *= acceleration;
        acceleration += 0.000001;

        // --- INPUT ---
        if (ispressed(VK_UP))
            paddle2.y -= 0.5;
        if (ispressed(VK_DOWN))
            paddle2.y += 0.5;
        if (ispressed('W'))
            paddle1.y -= 0.5;
        if (ispressed('S'))
            paddle1.y += 0.5;
        if (ispressed(VK_ESCAPE))
            break;
        // --- UPDATE ---
        if (paddle1.y < 0)
            paddle1.y = 0;
        if (paddle2.y < 0)
            paddle2.y = 0;
        if (paddle1.y + paddle1.height > HEIGHT)
            paddle1.y = HEIGHT - paddle1.height;
        if (paddle2.y + paddle2.height > HEIGHT)
            paddle2.y = HEIGHT - paddle2.height;
        ball.x += ball_dx;
        ball.y += ball_dy;
        if (ball.y <= 0 || ball.y + ball.height >= HEIGHT)
            ball_dy = -ball_dy;
        if (ball.collides(paddle1) || ball.collides(paddle2))
            ball_dx = -ball_dx;

        // --- RENDER ---
        screen.clear();
        screen.draw(ball, Color(255, 0, 0));
        screen.draw(paddle1, Color(255, 165, 0));
        screen.draw(paddle2, Color(255, 165, 0));
        screen.show();

        // --- FRAME LIMIT ---
        auto frame_time =
            chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - frame_start).count();

        if (frame_time < FRAME_TIME)
            this_thread::sleep_for(chrono::milliseconds(FRAME_TIME - frame_time));
    }

    return 0;
}