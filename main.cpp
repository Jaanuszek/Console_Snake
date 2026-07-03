#include <iostream>
#include <format>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <cstdint>
#include <random>
#include <signal.h>
#include <chrono>
#include <thread>
#include <termios.h>
#include <fcntl.h>

enum class Direction
{
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct position
{
    int32_t x{};
    int32_t y{};
    int32_t prev_x{};
    int32_t prev_y{};
};

std::string WIDTH_ARG_NAME = "--width";
std::string HEIGHT_ARG_NAME = "--height";

constexpr char FRUIT_CHAR = '@';
constexpr char SNAKE_CHAR = '*';
constexpr char EMPTY_CHAR = '=';

constexpr uint32_t FRUIT_COUNT = 5;

std::atomic<bool> gameIsRunning = true;

void safeExit();
void signalHandler(int sig);
void helpTemplate();
uint32_t getNumberFromStr(const std::string& str, const char c);
void moveCursor(int x, int y);

void addFruitsToMap(std::vector<std::vector<char>>& map,
                    std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation, 
                    uint32_t fruitCount);

void addFruitToMap(std::vector<std::vector<char>>& map,
                   std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation,
                   uint32_t index);

void removeFruitFromMap(std::vector<std::vector<char>>& map,
                        std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation,
                        uint32_t index);

void incrementSnakePos(Direction dir, int32_t& curr_w, int32_t& curr_h, uint32_t W, uint32_t H);
void moveSnake(std::vector<position>& snake, std::vector<std::vector<char>>& map, Direction dir, uint32_t W, uint32_t H);

Direction getDirectionFromKey(const std::array<char, 2>& seq);

void enableRawMode();
void disableRawMode();
void enableNonBlocking();
void disableNonBlocking();


int main(int argc, char* argv[])
{
    signal(SIGINT, signalHandler);
    enableRawMode();
    enableNonBlocking();

    uint32_t x{}, y{};

    for (int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
            helpTemplate();
            break;
        }
        else if(strncmp(argv[i], WIDTH_ARG_NAME.c_str(), WIDTH_ARG_NAME.length()) == 0)
        {
            std::string str(argv[i]);
            x = getNumberFromStr(str, '=');
        }
        else if(strncmp(argv[i], HEIGHT_ARG_NAME.c_str(), HEIGHT_ARG_NAME.length()) == 0)
        {
            std::string str(argv[i]);
            y = getNumberFromStr(str, '=');
        }
    }

    const uint32_t WIDTH = x > 0 ? x : 30;
    const uint32_t HEIGHT = y > 0 ? y : 10;
    const uint32_t TILE_COUNT = WIDTH * HEIGHT;
    const int32_t CENTER_X = WIDTH / 2;
    const int32_t CENTER_Y = HEIGHT / 2;

    std::vector<std::vector<char>> map(HEIGHT, std::vector<char>(WIDTH, EMPTY_CHAR));
    std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT> fruitsLocation;

    // std::list<position> head_node;

    addFruitsToMap(map, fruitsLocation, FRUIT_COUNT);

    // int32_t curr_w{}, curr_h{};
    std::vector<position> snakePosition;
    snakePosition.reserve(HEIGHT * WIDTH);
    snakePosition.push_back(position{.x = CENTER_X, .y = CENTER_Y});
    Direction curr_dir = Direction::UP;

    // wyczyszczenie planszy
    std::cout << "\033[2J";

    // wylaczenie kursora
    std::cout << "\033[?25l";

    // curr_h = CENTER_Y; curr_w = CENTER_X;
    while(gameIsRunning.load())
    {
        std::cout << "\033[H";
        for(auto& out : map)
        {
            for(auto& inner : out)
            {
                std::cout << inner;
            }
            std::cout << std::endl;
        }
        std::cout.flush();

        char c;
        if(read(STDIN_FILENO, &c, 1) > 0)
        {
            if(c == '\033')
            {
                std::array<char, 2> seq;
                read(STDIN_FILENO, &seq[0], 1);
                read(STDIN_FILENO, &seq[1], 1);
                curr_dir = getDirectionFromKey(seq);
            }
        }

        moveSnake(snakePosition, map, curr_dir, WIDTH, HEIGHT);

        uint32_t fruitIndex = 0;
        for(auto& [fruit_w, fruit_h] : fruitsLocation)
        {
            if(snakePosition[0].x == fruit_w && snakePosition[0].y == fruit_h)
            {
                std::cout << std::format("Fruit eaten at: ({}, {})\n", fruit_w, fruit_h);
                removeFruitFromMap(map, fruitsLocation, fruitIndex);
                addFruitToMap(map, fruitsLocation, fruitIndex);
                snakePosition.push_back(position{
                    .x = snakePosition[0].x,
                    .y = snakePosition[0].y
                });
                std::cout << snakePosition.size() << std::endl;
                break;
            }
            fruitIndex++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    safeExit();

    // wlaczenie kursora
    std::cout << "\033[?25h";
    return 0;
}

void signalHandler(int sig)
{
    if(sig == SIGINT)
    {
        safeExit();
    }
}

void safeExit()
{
    disableRawMode();
    disableNonBlocking();
    std::cout << "\033[?25h";
    exit(0);
}

void helpTemplate()
{
    std::cout << "1st argument: width" << std::endl;
    std::cout << "2nd argument: height" << std::endl;
    safeExit();
}

uint32_t getNumberFromStr(const std::string& str, const char c)
{
    size_t pos = str.find(c);
    std::string subString = str.substr(pos + 1);

    int num = std::stoi(subString);
    if(num < 0)
        return 0;

    return static_cast<uint32_t>(num);
}

void moveCursor(int x, int y) {
    std::cout << "\033[" << y + 1 << ";" << x + 1 << "H";
}

void addFruitsToMap(std::vector<std::vector<char>>& map, std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation, uint32_t fruitCount)
{

    // W tej funkcji jest taki problem ze za kazdym razem jak wywoluje ta funkcje to wynik jest zpisywany do pierwszego elementu fruitsLocation
    // no i jak inicjalne dodawanie owockow do mapy dziala, tak pojedyncze dodawnie owockow moze sie zepsuc
    // Dlatego albo nalezy dodac kolejna zmienna ktora przechowuje indeks aktualnie zjedzonego owocka, albo
    // dodac oddzielna funkcje ktora bedzie dodawala pojedyncze owocki do mapy i do fruitsLocation, a ta funkcja bedzie tylko do inicjalnego dodawania owockow
    uint32_t width = map[0].size();
    uint32_t height = map.size();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disWidth(0, width - 1);
    std::uniform_int_distribution<> disHeight(0, height - 1);

    for (uint32_t i = 0; i < fruitCount; ++i) {
        uint32_t rand_w = disWidth(gen);
        uint32_t rand_h = disHeight(gen);
        map[rand_h][rand_w] = FRUIT_CHAR;
        fruitsLocation[i] = {rand_w, rand_h};
    }
}

void addFruitToMap(std::vector<std::vector<char>>& map,
                   std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation,
                   uint32_t index)
{
    uint32_t width = map[0].size();
    uint32_t height = map.size();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disWidth(0, width - 1);
    std::uniform_int_distribution<> disHeight(0, height - 1);

    uint32_t rand_w = disWidth(gen);
    uint32_t rand_h = disHeight(gen);
    map[rand_h][rand_w] = FRUIT_CHAR;
    fruitsLocation[index] = {rand_w, rand_h};
}

void removeFruitFromMap(std::vector<std::vector<char>>& map,
                        std::array<std::pair<uint32_t, uint32_t>, FRUIT_COUNT>& fruitsLocation,
                        uint32_t index)
{
    if(index >= 0 && index < FRUIT_COUNT)
    {
        auto [fruit_w, fruit_h] = fruitsLocation[index];
        map[fruit_h][fruit_w] = EMPTY_CHAR;
        fruitsLocation[index] = {UINT32_MAX, UINT32_MAX};    
    }
}

void enableRawMode() {
    termios term{};

    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disableRawMode()
{
    termios term{};

    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag |= (ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void enableNonBlocking() {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void disableNonBlocking()
{
    fcntl(STDIN_FILENO, F_SETFL, 0);
}

void incrementSnakePos(Direction dir, int32_t& curr_w, int32_t& curr_h, uint32_t W, uint32_t H)
{
    switch(dir)
    {
        case Direction::UP:
            curr_h = (curr_h + H - 1) % H;
            break;
        case Direction::DOWN:
            curr_h = (curr_h + 1) % H;
            break;
        case Direction::LEFT:
            curr_w = (curr_w + W - 1) % W;
            break;
        case Direction::RIGHT:
            curr_w = (curr_w + 1) % W;
            break;
        default:
            break;
    }
}

void moveSnake(std::vector<position>& snakePos, std::vector<std::vector<char>>& map, Direction dir, uint32_t W, uint32_t H)
{
    // search the snake from head to tail
    // if iterator points to head, then move snake to proper direction
    // if iterator does not point to head, then move current node
    // to the position that next node contains
    for(auto it = snakePos.begin(); it != snakePos.end(); it++)
    {
        // As far as I understand, first iterator would point to head of the snake
        if(it == snakePos.begin())
        {
            auto next_it = std::next(it);
            if(next_it == snakePos.end())
            {
                map.at(it->y).at(it->x) = EMPTY_CHAR; 
            }

            // Store previous coordinates, so next nodes can hop in to the
            // previous node location
            it->prev_x = it -> x;
            it->prev_y = it -> y;

            incrementSnakePos(dir, it->x, it->y, W, H);

            // if next tile is a SNAKE_CHAR that means we hit the snake
            // so the game shall end
            if(map.at(it->y).at(it->x) == SNAKE_CHAR)
            {
                gameIsRunning = false;
                std::cout << "GAME OVER" << std::endl;
            }

            map.at(it->y).at(it->x) = SNAKE_CHAR;
        }
        else
        {
            // Because snake's head is at first elemnt of this vector
            // then the "next" element would be a previous one in this particular vector
            auto prev_it = std::prev(it);
            auto next_it = std::next(it);

            // If next_it points to the end of vector, then set this position as empty
            if(next_it == snakePos.end())
            {
                map.at(it->y).at(it->x) = EMPTY_CHAR; 
            }

            it->prev_x = it -> x;
            it->prev_y = it -> y;

            // next position of given node is a position of next (previous) node
            it->x = prev_it->prev_x;
            it->y = prev_it->prev_y;

            map.at(it->y).at(it->x) = SNAKE_CHAR;
        }
    }
}

Direction getDirectionFromKey(const std::array<char, 2>& seq)
{
    if(seq[0] == '[')
    {
        if(seq[1] == 'A')
        {
            return Direction::UP;
        }
        else if(seq[1] == 'B')
        {
            return Direction::DOWN;
        }
        else if(seq[1] == 'C')
        {
            return Direction::RIGHT;
        }
        else if(seq[1] == 'D')
        {
            return Direction::LEFT;
        }
    }
    return Direction::NONE;
}
