#include <bits/stdc++.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;

struct Rect
{
    double x, y, width, height;
    Rect() = default;
    Rect(double _x, double _y, double _w, double _h) : x(_x), y(_y), width(_w), height(_h) {}
    bool collides(Rect other)
    {
        return !(x + width <= other.x || other.x + other.width <= x || y + height <= other.y || other.y + other.height <= y);
    }
    bool operator==(const Rect other) const
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
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
    bool forced_to_update = false;
};

struct Texture
{
    int width, height;
    vector<Color> data;
    Texture(int w, int h) : width(w), height(h), data(w * h, Color(255, 255, 255)) {}
    Texture() = default;
    Texture(initializer_list<Color> list, int w, int h) : data(list), width(w), height(h) {}
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

    string canvas()
    {
        string c = u8"\033[48;2;255;255;255m";
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                c += u8" ";
            }
            c += "\n";
        }
        return c;
    }
    Rect round_values(Rect obj)
    {
        return {floor(obj.x), floor(obj.y), floor(obj.width), floor(obj.height)};
    }
    string changes()
    {
        int last_x, last_y;
        string changes;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (back_buffer.at(y * width + x) != front_buffer.at(y * width + x) || back_buffer.at(y * width + x).forced_to_update)
                {
                    if (x != last_x + 1 || y != last_y)
                    {
                        changes += move_cursor(x, y);
                    }
                    changes += back_buffer.at(y * width + x).upper_color.foreground();
                    changes += back_buffer.at(y * width + x).lower_color.background();
                    changes += u8"▀";
                    last_x = x, last_y = y;
                    back_buffer.at(y * width + x).forced_to_update = false;
                }
            }
        }
        return changes;
    }
    void force_to_update(Rect area)
    {
        area = round_values(area);
        for (int dy = 0; dy < area.height; dy++)
        {
            for (int dx = 0; dx < area.width; dx++)
            {
                if (area.y + dy >= height || area.x + dx >= width || area.y + dy < 0 || area.x + dx < 0)
                    continue;
                else
                    // back_buffer.at((area.y + dy) * width + (area.x + dx)).forced_to_update = true;
                    back_buffer.at((area.y + dy) * width + (area.x + dx)) = Cell(Color(1, 2, 3), Color(4, 5, 6)); // Hack to force update
            }
        }
    }

public:
    void print(string s, Color bg, Color text_color, int x, int y)
    {
        force_to_update(Rect(x, y, s.length(), 1));
        cout << move_cursor(x, y) << bg.background() << text_color.foreground() << s;
    }
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

    void clear()
    {
        fill(back_buffer.begin(), back_buffer.end(), Cell(Color(255, 255, 255), Color(255, 255, 255)));
    }

    void update()
    {
        cout << changes() << flush;
        front_buffer = back_buffer;
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

    Screen(int w, int h) : width(w),
                           height(h / 2),
                           back_buffer(height * width, Cell(Color(255, 255, 255), Color(255, 255, 255))),
                           front_buffer(height * width, Cell(Color(255, 255, 255), Color(255, 255, 255))) { cout << "\033[2J\033[?25l\033[H" << canvas(); }
};

bool ispressed(int vk_code)
{
    return GetAsyncKeyState(vk_code) & 0x8000;
}

void count_frames(std::chrono::_V2::steady_clock::time_point &frame_start, std::chrono::_V2::steady_clock::time_point &fps_clock, int &real_fps, int &frame_count)
{
    if (frame_start - fps_clock >= 1s)
    {
        fps_clock = frame_start;
        real_fps = frame_count;
        frame_count = 0;
    }
    else
    {
        frame_count++;
    }
}
void tick(std::chrono::_V2::steady_clock::time_point &frame_start, const int FPS)
{
    while (std::chrono::steady_clock::now() - frame_start < 1000ms / FPS)
    {
        _mm_pause();
    }
}

/// @brief Base class for creating a console game.
/// Inherit from this class and implement the update() and render() methods.
/// @note Use 'screen' member to draw on the console.
/// @note Call runs() method in the game loop to check if the game should continue running.
/// @note Call run() method in the game loop to update and render the game.
/// @note Press ESC key to exit the game.
/// @note Make sure to set the console to use UTF-8 encoding for proper rendering.
/// @note 'WIDTH' and 'HEIGHT' member variables define the console dimensions in characters.
class Game
{
private:
    int FPS, frame_count = 0, real_fps = 60;
    chrono::steady_clock::time_point fps_clock = chrono::steady_clock::now();

protected:
    Screen screen;
    int WIDTH, HEIGHT;

public:
    Game(int fps, int width, int height) : FPS(fps), WIDTH(width), HEIGHT(height), screen(width, height)
    {
        srand(time(nullptr));
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        cin.tie(nullptr);
        ios::sync_with_stdio(false);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    };
    bool runs()
    {
        return !ispressed(VK_ESCAPE);
    }
    void run()
    {
        auto frame_start = chrono::steady_clock::now();
        count_frames(frame_start, fps_clock, real_fps, frame_count);
        screen.clear();
        update();
        render();
        screen.print("FPS: " + to_string(real_fps), Color(0, 0, 0), Color(255, 255, 255), 0, 0);
        screen.update();
        tick(frame_start, FPS);
    }
    virtual void update() = 0;
    virtual void render() = 0;
};

//----------------------------------------------------------------------------------------------------------------------

struct Snake
{
    vector<Rect> segments;
    Rect head;
    Color head_color;
    Color segment_color;
    bool alive = true;
    int direction; // 0: up, 1: right, 2: down, 3: left
    Snake(int x, int y, int segment_size, Color head_color, Color segment_color) : direction(rand() % 4), head_color(head_color), segment_color(segment_color)
    {
        head = Rect(x, y, segment_size, segment_size);
        segments.push_back(Rect(x - segment_size, y, segment_size, segment_size));
    }
};

class SnakeGame : public Game
{
    int segment_size = 2;
    Snake snake1 = Snake(rand() % clamp(WIDTH / segment_size, 1, WIDTH / segment_size) * segment_size, rand() % clamp(HEIGHT / segment_size, 1, HEIGHT / segment_size) * segment_size, segment_size, Color(0, 255, 0), Color(0, 200, 0));
    Rect food = Rect(rand() % (WIDTH / segment_size) * segment_size, rand() % (HEIGHT / segment_size) * segment_size, segment_size, segment_size);

public:
    SnakeGame(int fps, int width, int height) : Game(fps, width, height) {}
    void update() override
    {
        if (snake1.alive)
        {
            if (ispressed(VK_UP) && snake1.direction != 2)
                snake1.direction = 0;
            else if (ispressed(VK_RIGHT) && snake1.direction != 3)
                snake1.direction = 1;
            else if (ispressed(VK_DOWN) && snake1.direction != 0)
                snake1.direction = 2;
            else if (ispressed(VK_LEFT) && snake1.direction != 1)
                snake1.direction = 3;

            for (const auto &segment : snake1.segments)
            {
                if (snake1.head.collides(segment))
                {
                    snake1.alive = false;
                    break;
                }
            }

            bool ate_food = false;
            if (snake1.head.collides(food))
            {
                food = Rect(rand() % (WIDTH / segment_size) * segment_size, rand() % (HEIGHT / segment_size) * segment_size, segment_size, segment_size);
                while (find(snake1.segments.begin(), snake1.segments.end(), food) != snake1.segments.end() || snake1.head == food)
                {
                    food = Rect(rand() % (WIDTH / segment_size) * segment_size, rand() % (HEIGHT / segment_size) * segment_size, segment_size, segment_size);
                }
                ate_food = true;
            }
            snake1.segments.insert(snake1.segments.begin(), snake1.head);
            switch (snake1.direction)
            {
            case 0:
                snake1.head = {Rect(snake1.head.x, ((int)snake1.head.y - segment_size + HEIGHT) % HEIGHT, segment_size, segment_size)};
                break;
            case 1:
                snake1.head = {Rect(((int)snake1.head.x + segment_size) % WIDTH, snake1.head.y, segment_size, segment_size)};
                break;
            case 2:
                snake1.head = {Rect(snake1.head.x, ((int)snake1.head.y + segment_size) % HEIGHT, segment_size, segment_size)};
                break;
            case 3:
                snake1.head = {Rect(((int)snake1.head.x - segment_size + WIDTH) % WIDTH, snake1.head.y, segment_size, segment_size)};
                break;
            }
            if (!ate_food)
            {
                snake1.segments.pop_back();
            }
        }
    }
    void render() override
    {
        screen.draw(Rect(0, 0, WIDTH, HEIGHT), Color(0, 0, 0));
        screen.draw(food, Color(255, 0, 0));
        if (snake1.alive)
        {
            screen.draw(snake1.head, snake1.head_color);
            for (const auto &segment : snake1.segments)
            {
                screen.draw(segment, snake1.segment_color);
            }
        }
        screen.print("Score: " + to_string(snake1.segments.size() - 1), Color(0, 0, 0), Color(0, 255, 0), 0, 1);
    }
};

int main()
{

    const auto FPS = 10, WIDTH = 120, HEIGHT = 29 * 2;
    auto game = SnakeGame(FPS, WIDTH, HEIGHT);

    while (game.runs())
    {
        game.run();
    }
    return 0;
}
