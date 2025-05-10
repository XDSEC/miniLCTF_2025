#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

class CTFer {
  protected:
    const long points;
    std::string nickname;

  public:
    CTFer(const long points, std::string name)
        : points(points), nickname(name) {}
    virtual void info() const = 0;
    virtual ~CTFer() = default;
};

class Web : public CTFer {
  public:
    void info() const override {
        std::cout << "I am " << this->nickname << ", "
                  << "I work on web exploitation and gained " << this->points
                  << " points.\n";
    }
    Web(const long points, std::string name) : CTFer(points, name) {}
    ~Web() {}
};

class Binary : public CTFer {
  public:
    void info() const override {
        std::cout << "I am " << this->nickname << ", "
                  << "I work on binary exploitation and gained " << this->points
                  << " points.\n";
    }
    Binary(const long points, std::string name) : CTFer(points, name) {}
    ~Binary() {}
};

std::vector<CTFer *> ctfers;
char name_buf[128] = {0};

static void join_ctf() {
    long points;
    bool is_binary;
    std::cout << "Name > ";
    std::cin.ignore();
    std::cin >> name_buf;
    std::cout << "Points > ";
    std::cin >> points;
    std::cout << "Type: Web - 0 / Binary - 1 > ";
    std::cin >> is_binary;
    if (is_binary) {
        ctfers.push_back(static_cast<CTFer *>(new Binary(points, name_buf)));
    } else {
        ctfers.push_back(static_cast<CTFer *>(new Web(points, name_buf)));
    }
    std::cout << "CTFer " << name_buf << " joined!\n";
}

static void quit_ctf() {
    size_t idx;
    std::cout << "Index > ";
    std::cin >> idx;
    CTFer *target;
    try {
        target = ctfers.at(idx);
    } catch (const std::out_of_range &oor) {
        std::cout << "CTFer " << idx << " not found!\n";
        return;
    }
    ctfers.erase(ctfers.begin() + idx);
    std::cout << "CTFer " << idx << " removed!\n";
}

static void print_info() {
    for (size_t i = 0; i < ctfers.size(); ++i) {
        std::cout << '[' << i << "] ";
        ctfers[i]->info();
    }
}

static void init() { std::setbuf(stdout, nullptr); }

static void menu() {
    std::cout << "[CTF Participants Management System]\n0. Join CTF\n1. Quit "
                 "CTF\n2. Print info\n3. Exit.\n";
}

bool broken = false;

static void bug() {
    if (ctfers.size() == 0 || broken) {
        return;
    }
    std::cin >> *(size_t *)&ctfers[0];
    broken = true;
}

int main() {
    unsigned int choice;
    init();
    for (;;) {
        menu();
        std::cout << "Choice > ";
        std::cin >> choice;
        switch (choice) {
        case 0:
            join_ctf();
            break;
        case 1:
            quit_ctf();
            break;
        case 2:
            print_info();
            break;
        case 3:
            std::exit(EXIT_SUCCESS);
        case 0xdeadbeef:
            bug();
        default:
            std::cout << "Invalid choice!\n";
            break;
        }
    }
}

// Built with ubuntu:22.04 g++ 11.4.0
// g++ -mtune=generic -pipe -fno-plt -fexceptions -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -Wp,-D_GLIBCXX_ASSERTIONS -no-pie main.cpp -o ctfers
