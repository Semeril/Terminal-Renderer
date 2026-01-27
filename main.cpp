#include <bits/stdc++.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;

enum State
{
    PONG,
    START
};

struct Rect
{
    double x, y, width, height;
    Rect() = default;
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

struct Cell
{
    Color upper_color;
    Color lower_color;
    Cell() = default;
    Cell(Color upper, Color lower) : upper_color(upper), lower_color(lower) {}
    bool operator!=(const Cell other) const
    {
        return upper_color != other.upper_color || lower_color != other.lower_color;
    }
};

struct Texture
{
    int width, height;
    vector<Color> data;
    Texture(int w, int h) : width(w), height(h), data(w * h, Color(255, 255, 255)) {}
    Texture() = default;
    Texture(initializer_list<Color> list, int w, int h) : data(list), width(w), height(h) {}
};

struct Text
{
    string s;
};

string move_cursor(int x, int y)
{
    return "\033[" + to_string(y + 1) + ";" + to_string(x + 1) + "H";
}

class Screen
{
    int width, height;
    vector<Cell> back_buffer, front_buffer;
    void setpx(int x, int y, Color c)
    {
        if (y % 2)
        {
            back_buffer.at((y / 2) * width + x).lower_color = c;
        }
        else
        {
            back_buffer.at((y / 2) * width + x).upper_color = c;
        }
    }

public:
    void draw(Rect rect, Color color)
    {
        rect = round_values(rect);
        for (int dy = 0; dy < rect.height; dy++)
        {
            for (int dx = 0; dx < rect.width; dx++)
            {
                if (rect.y + dy >= height * 2 || rect.x + dx >= width || rect.y + dy < 0 || rect.x + dx < 0)
                    continue;
                else
                    setpx(rect.x + dx, rect.y + dy, color);
            }
        }
    }

    Rect round_values(Rect obj)
    {
        return {floor(obj.x), floor(obj.y), floor(obj.width), floor(obj.height)};
    }

    void clear()
    {
        fill(back_buffer.begin(), back_buffer.end(), Cell(Color(255, 255, 255), Color(255, 255, 255)));
    }

    void update()
    {
        front_buffer = back_buffer;
    }

    string canvas()
    {
        string c = u8"\033[48;2;255;255;255m";
        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                c += u8" ";
            }
            c += "\n";
        }
        return c;
    }

    void draw(Texture texture, double x_pos, double y_pos)
    {
        int x_start = round(x_pos), y_start = round(y_pos);
        for (int y = 0; y < texture.height; y++)
        {
            for (int x = 0; x < texture.width; x++)
            {
                if (y_start + y >= height * 2 || x_start + x >= width || y_start + y < 0 || x_start + x < 0)
                    continue;
                else
                    setpx(x_start + x, y_start + y, texture.data.at(y * texture.width + x));
            }
        }
    }

    string changes()
    {
        int last_x, last_y;
        string changes;
        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                if (back_buffer.at(y * width + x) != front_buffer.at(y * width + x))
                {
                    if (x != last_x + 1 || y != last_y)
                    {
                        changes += move_cursor(x, y);
                    }
                    changes += back_buffer.at(y * width + x).upper_color.foreground();
                    changes += back_buffer.at(y * width + x).lower_color.background();
                    changes += u8"▀";
                    last_x = x, last_y = y;
                }
            }
        }
        return changes;
    }

    Screen(int w, int h) : width(w),
                           height(h / 2),
                           back_buffer(height * width, Cell(Color(255, 255, 255), Color(255, 255, 255))),
                           front_buffer(height * width, Cell(Color(255, 255, 255), Color(255, 255, 255))) {}
};

bool ispressed(int vk_code)
{
    return GetAsyncKeyState(vk_code) & 0x8000;
}

int main()
{

    const auto FPS = 60, FRAME_TIME = 1000 / FPS, WIDTH = 120, HEIGHT = 29 * 2;
    {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        cin.tie(nullptr);
        ios::sync_with_stdio(false);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    Screen screen(WIDTH, HEIGHT);
    Rect ball(WIDTH / 2, HEIGHT / 2, 2, 2), paddle1(2, HEIGHT / 2 - 3, 2, 12), paddle2(WIDTH - 4, HEIGHT / 2 - 3, 2, 12);
    double ball_dx = 0.5, ball_dy = 0.5;
    double acceleration = 1.00001;
    State state = START;

    cout << "\033[2J\033[?25l\033[H"; // Clear screen and hide cursor
    int64_t frame_time = 16;
    int fps_count = 0;
    int fps = 60;
    auto fps_clock = chrono::steady_clock::now();

    cout << screen.canvas();

    while (true)
    {
        auto frame_start = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - fps_clock).count() >= 1)
        {
            fps_clock = chrono::steady_clock::now();
            fps = fps_count;
            fps_count = 0;
        }
        else
        {
            fps_count++;
        }
        screen.clear();
        switch (state)
        {
        case START:
            cout << move_cursor(WIDTH / 2, HEIGHT / 4) << Color(0, 0, 0).foreground() << "Pong Game";
            break;
        case PONG:
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

            ball.x += ball_dx;
            ball.y += ball_dy;
            if (ball.y <= 0)
            {
                ball.y = 0;
                ball_dy = -ball_dy;
            }
            if (ball.y + ball.height > HEIGHT)
            {
                ball.y = HEIGHT - ball.height;
                ball_dy = -ball_dy;
            }
            if (ball.collides(paddle1))
            {
                ball.x = paddle1.x + paddle1.width;
                ball_dx = -ball_dx;
            }
            if (ball.collides(paddle2))
            {
                ball.x = paddle2.x - ball.width;
                ball_dx = -ball_dx;
            }

            // --- RENDER ---

            screen.draw(Rect(0, 0, WIDTH, HEIGHT), Color(0, 0, 0));
            screen.draw(ball, Color(255, 0, 0));
            screen.draw(paddle1, Color(255, 165, 0));
            screen.draw(paddle2, Color(255, 165, 0));
            cout << move_cursor(0, 0);
            cout << Color(0, 0, 0).background() << Color(255, 255, 255).foreground(); // Reset colors
            cout << "FPS: " << fps;
            break;
        }

        cout << screen.changes() << flush;
        screen.update();

        // --- FRAME LIMIT ---
        frame_time =
            chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - frame_start).count();

        while (std::chrono::steady_clock::now() - frame_start < std::chrono::milliseconds(FRAME_TIME + 1))
        {
            _mm_pause(); // For CPU efficiency
        }
    }

    return 0;
}