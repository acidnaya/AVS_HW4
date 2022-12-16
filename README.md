# Рубцова М.Е., БПИ-218, ИДЗ 4, Вариант 22.

## Задание

Задача об инвентаризации по книгам. После нового года в библиотеке
университета обнаружилась пропажа каталога. После поиска и наказания, виноватых ректор дал указание восстановить каталог силами студентов. Фонд
библиотека представляет собой прямоугольное помещение, в котором находится M рядов по N шкафов по K книг в каждом шкафу. Требуется создать
многопоточное приложение, составляющее каталог. При решении задачи использовать метод «портфель задач», причем в качестве отдельной
задачи задается внесение в каталог записи об отдельной книге. Примечание. Каталог — это список книг, упорядоченный по их инвентарному номеру
или по алфавиту (на выбор разработчика). Каждая строка каталога содержит идентифицирующее значение (номер или название), номер ряда, номер
шкафа, номер книги в шкафу.
 
 ## Решение

### Исходный код на языке С на 6 баллов:
```c
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

// функция считывающая пользовательский ввод – положительное целое число меньше заданной границы
int get_positive_number(int bound, std::string s) {
    int a = -1;
    while (a < 1 || a > bound) {
        std::cout << "Enter a positive number in [1; " << bound << "] – amount of " << s << ":\n";
        std::cin >> a;
    }
    return a;
}

// функция генерирующая книги в рандомном порядке на полках библиотеки
std::vector<book> generate_books(int rows, int shelfs, int positions) {
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
        // студент-поток вносит запись в каталог
        catalog.insert(std::pair<int, book>(current_book.id, current_book));
        pthread_mutex_unlock(&map_mutex);
    }
}

// функция выводящая одну запись из каталога
void print_book_info(book b) {
    std::cout << "ID:\t" << b.id << ",\tROW:\t" << b.row << ",\tSHELF:\t" << b.shelf << ",\tPOS:\t" << b.position <<"\t(by Student " << b.student << ")\n";
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
    } else {
        // консольный ввод
        rows = get_positive_number(100, "rows");
        shelfs = get_positive_number(100, "shelfs");
        positions = get_positive_number(100, "positions");
        threads = get_positive_number(8, "students");
    }

    // инициализация переменных, которые будут использоваться потоком
    initialize_globals(rows, shelfs, positions);

    // массив заданного числа потоков
    pthread_t students[threads];
    // массив идентификаторов потоков
    int thread[threads];

    for (int i = 0; i < threads; ++i) {
        // заполнение идентификаторов
        thread[i] = i + 1;
        // создание потоков (в качестве аргумента передаем идентификатор потока)
        pthread_create(&students[i], nullptr, catalog_book, (void *) &(thread[i]));
    }

    for (int i = 0; i < threads; ++i) {
        // запуск работы потоков
        pthread_join(students[i], nullptr);
    }

    // вывод каталога
    printf("* * * CATALOG * * *\n");
    for (const std::pair<int, book>& items : catalog) {
        print_book_info(items.second);
    }
    return 0;
}
```

Компиляция:
```sh
g++ -o prog -lpthread main.cpp
```

### (4 балла) Описана модель параллельных вычислений, используемая при разработке многопоточной программы.
Использована модель «взаимодействующие равные», в которой исключен управляющий поток, не занимающийся непосредственными вычислениями. Распределение работ в данной задаче динамически определяется во время выполнения с помощью «портфеля задач». «Портфель задач» реализован с помощью разделяемой переменной (portfolio_task), доступ к которой в один момент времени имеет только один процесс.
Вычислительная задача делится на конечное число подзадач. Подзадачи нумеруются, и каждому номеру определяется функция, которая однозначно отражает номер задачи на соответствующий ему набор данных. Каждый поток сначала обращается к портфелю задач для выяснения текущего номера задачи, после этого увеличивает его, потом берет соответствующие данные и выполняет задачу, затем обращается к портфелю задач для выяснения следующего номера задачи.
### (4 балла) Описаны входные данные программы, включающие вариативные диапазоны, возможные при многократных запусках.
Целое число в диапазоне [1, 100] - количество рядов в библиотеке.\
Целое число в диапазоне [1, 100] - количество полок в ряду.\
Целое число в диапазоне [1, 100] - количество мест на полке.\
Целое число в диапазоне [1, 8] - количество студентов.
Возможен ввод из командной строки в формате:
```sh
./prog rows_number bookshelfs_number positions_number students_number
```
Если количество аргументов форматной строки не равно 4, то будет запущен консольный ввод. Если аргументы не являются числами или находятся в недопустимом диапазоне, то исполнение программы прервется. Номера книг и их позиции в библиотеке будут сгенерированы в соответствии с входными данными.
### (4 балла) Реализовано консольное приложение, решающее поставленную задачу с использованием одного варианта синхропримитивов.
Код представлен выше. Использовались мьютексы.
### (4 балла) Ввод данных в приложение реализован с консоли.
Реализован.
### (5 баллов) В программу добавлены комментарии, поясняющие выполняемые действия и описание используемых переменных.
С комментариями можно ознакомится выше.
### (5 баллов) Сценарий, описывающий одновременное поведение представленных в условии задания сущностей в терминах предметной области. То есть, описано поведение объектов разрабатываемой программы как взаимодействующих субъектов, а не то, как это будет реализовано в программе.
Субъекты: студенты, библиотека с рядами, шкафами и полками, каталог.
В роли «портфеля задач» в моем случае выступает некий идентификатор позиции книги(не номер и не местоположение, это просто условно СЛЕДУЮЩАЯ неучтенная книга). Можно представить, что под каждой книгой есть наклейка с номером от 0 до количества всех книг в библиотеке, есть автомат который по запросу студента выдает тому такой номер и сразу же увеличивает этот номер на 1 до тех пор, пока номер не превысит кол-во книг. В общем, это что-то что держит в курсе всех студентов о том, какая книга еще не учтена.
1) Студент берет талон с номером книги из автомата. Если автомат занят, то студент ждет своей очереди
2) Студент находит по этому номеру еще неучтенную книгу и запоминает ее местоположение.
3) Cтудент идет к каталогу по пути изображая бурную деятельность и записывает в каталог местоположение (ряд, шкаф, полку) книги.
4) Студент возвращается за номером следующей неучтенной книги к автомату.
5) Это повторяется со множеством студентов до тех пор, пока все книги не будут внесены в каталог.

### (6 баллов) Подробно описан обобщенный алгоритм, используемый при реализации программы исходного словесного сценария. В котором показано, как на программу отображается каждый из субъектов предметной области.
### (6 баллов) Реализован ввод данных из командной строки
Реализован.

### Тестирование

![Тестирование](https://github.com/acidnaya/AVS_HW3/blob/main/screenshots/testing11.png)
