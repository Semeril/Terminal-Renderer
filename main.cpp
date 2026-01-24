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
        return !(x + width <= other.x || other.x + other.width <= x || y + height <= other.y || other.y + other.height <= y);
    }
};

struct Color
{
    uint8_t r, g, b;

    string background()
    {
        return "\033[48;2;" + to_string(r) + ';' + to_string(g) + ';' + to_string(b) + 'm';
    }
    string foreground()
    {
        return "\033[38;2;" + to_string(r) + ';' + to_string(g) + ';' + to_string(b) + 'm';
    }
    bool operator!=(const Color other) const
    {
        return r != other.r || g != other.g || b != other.b;
    }
    Color() = default;
    Color(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}
};

struct ColoredChar
{
    char c;
    Color background_color;
    Color foreground_color;
    ColoredChar(char _c, Color bg, Color fg) : c(_c), background_color(bg), foreground_color(fg) {}
};

struct Text
{
    string content;
    Color color;
    Text(string _content, Color _color) : content(_content), color(_color) {}
};

string move_cursor(int x, int y)
{
    return "\033[" + to_string(y + 1) + ";" + to_string(x + 1) + "H";
}

class Screen
{
    int width, height;
    vector<ColoredChar> back_buffer, front_buffer;

public:
    void draw(Rect rect, Color color)
    {
        rect = scale(rect);
        for (int dy = 0; dy < rect.height; dy++)
        {
            for (int dx = 0; dx < rect.width; dx++)
            {
                if (rect.y + dy >= height || rect.x + dx >= width || rect.y + dy < 0 || rect.x + dx < 0)
                    continue;
                else
                    back_buffer.at((rect.y + dy) * width + (rect.x + dx)) = ColoredChar(' ', color, color);
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
            back_buffer.at(py * width + px) = ColoredChar(ch, back_buffer.at(py * width + px).background_color, text.color);
            px++;
        }
    }
    Rect scale(Rect obj)
    {
        obj.x = round(obj.x * 2);
        obj.y = round(obj.y);
        obj.width = round(obj.width * 2);
        obj.height = round(obj.height);
        return obj;
    }
    void clear()
    {
        fill(back_buffer.begin(), back_buffer.end(), ColoredChar(' ', Color(255, 255, 255), Color(255, 255, 255)));
    }
    void update()
    {
        front_buffer = back_buffer;
    }
    string canvas()
    {
        string c = "\033[48;2;255;255;255m";
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                c += " ";
            }
            c += "\n";
        }
        return c;
    }
    string changes()
    {
        string changes;
        changes.reserve(width * height * 20);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (front_buffer.at(y * width + x).c != back_buffer.at(y * width + x).c ||
                    front_buffer.at(y * width + x).background_color != back_buffer.at(y * width + x).background_color ||
                    front_buffer.at(y * width + x).foreground_color != back_buffer.at(y * width + x).foreground_color)
                {
                    changes += move_cursor(x, y);
                    changes += back_buffer.at(y * width + x).background_color.background();
                    changes += back_buffer.at(y * width + x).foreground_color.foreground();
                    changes += back_buffer.at(y * width + x).c;
                }
            }
        }
        return changes;
    }
    int changes_len()
    {
        int cnt = 0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (front_buffer.at(y * width + x).c != back_buffer.at(y * width + x).c ||
                    front_buffer.at(y * width + x).background_color != back_buffer.at(y * width + x).background_color ||
                    front_buffer.at(y * width + x).foreground_color != back_buffer.at(y * width + x).foreground_color)
                {
                    cnt++;
                }
            }
        }
        return cnt;
    }

    Screen(int w, int h) : width(w),
                           height(h),
                           back_buffer(height * width, ColoredChar(' ', Color(255, 255, 255), Color(255, 255, 255))),
                           front_buffer(height * width, ColoredChar(' ', Color(255, 255, 255), Color(255, 255, 255))) {}
};

bool ispressed(int vk_code)
{
    return GetAsyncKeyState(vk_code) & 0x8000;
}

int main()
{
    cin.tie(nullptr);
    ios::sync_with_stdio(false);
    srand((unsigned)time(nullptr));

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
    cout << "\033[2J\033[?25l\033[H"; // Clear screen and hide cursor
    int64_t frame_time = 16;

    cout << screen.canvas();

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
        int it = 5;
        for (int i = 0; i < it; i++)
        {
            ball.x += ball_dx / it;
            ball.y += ball_dy / it;
            if (ball.y <= 0 || ball.y + ball.height > HEIGHT)
            {
                ball.y = clamp(ball.y, 0.0, HEIGHT - ball.height);
                ball_dy = -ball_dy;
            }
            if (ball.collides(paddle1) || ball.collides(paddle2))
            {
                ball_dx = -ball_dx;
                ball_dy += ((rand() % 100) / 100.0 - 0.5) * 0.5;
            }
        }

        // --- RENDER ---
        screen.clear();
        screen.draw(Rect(0, 0, RECT_WIDTH, HEIGHT), Color(0, 0, 0));
        screen.draw(ball, Color(255, 0, 0));
        screen.draw(paddle1, Color(255, 165, 0));
        screen.draw(paddle2, Color(255, 165, 0));
        screen.draw(Text("Frame time: " + to_string(frame_time), Color(0, 255, 255)), 0, 0);
        screen.draw(Text("Changes: ~" + to_string(screen.changes_len() - 11), Color(0, 255, 255)), 0, 1);

        cout << screen.changes() << flush;
        screen.update();

        // --- FRAME LIMIT ---
        frame_time =
            chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - frame_start).count();

        if (frame_time < FRAME_TIME)
            this_thread::sleep_for(chrono::milliseconds(FRAME_TIME - frame_time));
    }

    return 0;
}