#include <map>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <random>
#include <time.h>

// книга, содержащая в себе номер, ряд, полку и позицию,
// а также идентификатор студента-потока, внесшего о ней запись в каталог
struct book{
    int id;
    int row;
    int shelf;
    int position;
    int student;

    book() {}

    book(int i, int r, int s, int p) {
        id = i;
        row = r;
        shelf = s;
        position = p;
        student = 0;
    }
};

// портфель задач, который выдает потоку номер задачи = порядковый номер книги в библиотеке
int portfolio_task;

// библиотека или же хранилище данных для каждой задачи
std::vector<book> library;

// каталог, куда будут записываться книги в порядке возрастания book.id
std::map<int, book> catalog;

// mutex для портфеля задач, обеспечивает безопасность взаимодействия с ним потоков
pthread_mutex_t portfolio_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex для добавления в каталог, обеспечивает безопасность взаимодействия с ним потоков
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

// функция генерирующая книги в рандомном порядке на полках библиотеки
std::vector<book> generate_books(int rows, int shelfs, int positions) {
    std::cout << "Generating books in library...\n";
    std::vector<int> v;
    std::vector<book> books;
    int books_amount = rows * shelfs * positions;
    for (int i = 1; i <= books_amount; ++i) {
        v.push_back(i);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    for (int r = 1; r <= rows; ++r) {
        for (int s = 1; s <= shelfs; ++s) {
            for (int p = 1; p <= positions; ++p) {
                std::uniform_int_distribution<> distr(0, v.size() - 1);
                int i = distr(gen);
                books.push_back(book(v[i], r, s, p));
                v.erase(v.begin() + i);
            }
        }
    }
    std::cout << "Books are generated.\n";
    return books;
}

// инициализатор глобальных переменных
void initialize_globals(int rows, int shelfs, int positions) {
    portfolio_task = 0;
    library = generate_books(rows, shelfs, positions);
}

// функция каталогизирования для потоков
void *catalog_book(void *arg) {
    int current_task;
    int *student = (int *)arg;
    book current_book;
    while (true) {
        pthread_mutex_lock(&portfolio_mutex);
        current_task = portfolio_task; // получение потоком из портфеля задач номера задачи
        portfolio_task++; // увеличение счетчика задач
        pthread_mutex_unlock(&portfolio_mutex);
        if (current_task >= library.size()) { // проверка на условия завершения работы
            return nullptr; // завершение работы если проверка пройдена
        }
        // иначе поток продолжает работу, получает данные о книге в библиотеке
        current_book = library[current_task];
        // имитация бурной деятельности для репрезентативности вывода
        timespec time;
        time.tv_sec = 0;
        time.tv_nsec = 100 / *student;
        nanosleep(&time, nullptr);
        // студент-поток записывает свой идентификатор (для дальнейшего вывода)
        current_book.student = *student;
        pthread_mutex_lock(&map_mutex);
        std::cout << "Student " << *student << " wrote book # " << current_book.id << "\tinto catalog.\n";
        // студент-поток вносит запись в каталог
        catalog.insert(std::pair<int, book>(current_book.id, current_book));
        pthread_mutex_unlock(&map_mutex);
    }
}

// функция выводящая одну запись из каталога
void print_book_info(book b) {
    std::cout << "ID:\t" << b.id << ",\tROW:\t" << b.row << ",\tSHELF:\t" << b.shelf << ",\tPOS:\t" << b.position <<"\t(by Student " << b.student << ")\n";
}

// функция валидирующая параметры
bool validate(int number, int boundary) {
    if (number < 1 || number > boundary) {
        return false;
    }
    return true;
}

// функция считывающая пользовательский ввод – положительное целое число меньше заданной границы
int get_positive_number(int boundary, std::string s) {
    int a = -1;
    while (!validate(a, boundary)) {
        std::cout << "Enter a positive number in [1; " << boundary << "] – amount of " << s << ":\n";
        std::cin >> a;
    }
    return a;
}

int main(int argc, char *argv[]) {
    int rows;
    int shelfs;
    int positions;
    int threads;
    if (argc == 5) {
        // ввод данных из командной строки
        rows = atoi(argv[1]);
        shelfs = atoi(argv[2]);
        positions = atoi(argv[3]);
        threads = atoi(argv[4]);
        if (!(validate(rows, 100) && validate(shelfs, 100) && validate(positions, 100) && validate(threads, 100))) {
            std::cout << "Incorrect input.\n";
            return 0;
        }
    } else {
        // консольный ввод
        rows = get_positive_number(100, "rows");
        shelfs = get_positive_number(100, "shelfs");
        positions = get_positive_number(100, "positions");
        threads = get_positive_number(100, "students");
    }

    // инициализация переменных, которые будут использоваться потоком
    initialize_globals(rows, shelfs, positions);
    pthread_mutex_init(&portfolio_mutex, nullptr);
    pthread_mutex_init(&map_mutex, nullptr);

    // массив заданного числа потоков
    pthread_t students[threads];
    // массив идентификаторов потоков
    int thread[threads];
    std::cout << "Students have joined work.\n";
    for (int i = 0; i < threads; ++i) {
        // заполнение идентификаторов
        thread[i] = i + 1;
        // создание потоков (в качестве аргумента передаем идентификатор потока)
        pthread_create(&students[i], nullptr, catalog_book, (void *) &(thread[i]));
    }

    for (int i = 0; i < threads; ++i) {
        // завершение работы потоков
        pthread_join(students[i], nullptr);
    }
    pthread_mutex_destroy(&portfolio_mutex);
    pthread_mutex_destroy(&map_mutex);
    std::cout << "Students have completed work.\n";

    // вывод каталога
    printf("\n* * * * * * * * * * * *  CATALOG  * * * * * * * * * * * *\n");
    for (const std::pair<int, book>& items : catalog) {
        print_book_info(items.second);
    }
    return 0;
}
