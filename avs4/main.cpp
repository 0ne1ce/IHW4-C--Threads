#include <iostream>
#include <pthread.h>
#include <queue>
#include <random>
#include <unistd.h>
#include <cstdlib>

pthread_mutex_t mutex;         // Мьютекс для защиты общих ресурсов
pthread_cond_t room_available; // Условная переменная для ожидания освобождения номеров

int NUM_ROOMS = 25;             // Количество номеров в гостинице (ввод с консоли)
int NUM_CLIENTS = 50;           // Количество клиентов (ввод с консоли)
int available_rooms;            // Текущее количество доступных номеров
std::queue<int> waiting_queue;  // Очередь клиентов, ожидающих номер

int min_stay = 1;               // Минимальное количество дней пребывания
int max_stay = 5;               // Максимальная длительность

// Функция для потока клиента
void* client_thread(void* arg) {
  int client_id = *(int*)arg;

  // Генератор случайных чисел.
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(min_stay, max_stay);

  // Определяем случайную длительность пребывания
  int stay_duration = distr(gen);

  // Клиент пытается заселиться
  pthread_mutex_lock(&mutex);

  std::cout << "Клиент " << client_id << " приехал в гостиницу и хочет остаться на "
            << stay_duration << " дн.\n";

  if (available_rooms == 0) { // Проверка наличия свободных номеров. Если их нет, то клиент встает в очередь
    std::cout << "Клиент " << client_id << " не нашел свободных номеров и ждет на кресле.\n";
    waiting_queue.push(client_id);
    while (waiting_queue.front() != client_id || available_rooms == 0) { // Клиент ждет своей очереди и хотя бы один свободный номер
      pthread_cond_wait(&room_available, &mutex);
    }
    waiting_queue.pop(); // Клиент получил очередь на заселение, выходим из очереди
  }

  // Клиент, который может занять номер (изначально или после выхода из очереди)
  available_rooms--;
  std::cout << "Клиент " << client_id << " заселяется в номер. Свободных номеров осталось: "
            << available_rooms << ".\n";

  pthread_mutex_unlock(&mutex);

  // Клиент проводит в номере заданное количество "дней" (секунд)
  sleep(stay_duration);

  // Клиент освобождает номер
  pthread_mutex_lock(&mutex);
  available_rooms++;
  std::cout << "Клиент " << client_id << " выехал из номера. Свободных номеров стало: "
            << available_rooms << ".\n";

  // Оповещаем всех ожидающих в очереди, что номер освободился
  pthread_cond_broadcast(&room_available);
  pthread_mutex_unlock(&mutex);

  return nullptr;
}

int main(int argc, char* argv[]) {
  // Ввод начальных данных в командной строке
  // Создание исполняемого файла с помощью команды: g++ -pthread main.cpp hotel
  // (hotel - имя параметр, можно задать другое, но его же использовать в следующих командах))
  // Формат: ./hotel_simulator [NUM_ROOMS] [NUM_CLIENTS] [MIN_STAY] [MAX_STAY]
  if (argc > 1) NUM_ROOMS = std::atoi(argv[1]);
  if (argc > 2) NUM_CLIENTS = std::atoi(argv[2]);
  if (argc > 3) min_stay = std::atoi(argv[3]);
  if (argc > 4) max_stay = std::atoi(argv[4]);

  if (min_stay > max_stay) {
    std::cerr << "Ошибка: минимальная длительность должна быть больше максимальной!";
    return 1;
  }
  else if (min_stay < 1 || max_stay < 1) {
    std::cerr << "Ошибка: длительность пребывания должна быть положительным числом!";
    return 1;
  }
  else if (NUM_ROOMS < 1) {
    std::cerr << "Ошибка: количество номеров в гостинице должно быть положительным числом!";
    return 1;
  }
  else if (NUM_CLIENTS < 1) {
    std::cerr << "Ошибка: количество клиентов должно быть положительным числом!";
    return 1;
  }



  available_rooms = NUM_ROOMS; // Количество свободных номеров

  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&room_available, nullptr);

  // Создаем потоки (клиентов)
  auto clients = new pthread_t[NUM_CLIENTS];
  int* client_ids = new int[NUM_CLIENTS];

  for (int i = 0; i < NUM_CLIENTS; i++) {
    client_ids[i] = i + 1;
    pthread_create(&clients[i], nullptr, client_thread, &client_ids[i]);
  }

  // Ожидаем завершения всех потоков
  for (int i = 0; i < NUM_CLIENTS; i++) {
    pthread_join(clients[i], nullptr);
  }

  // Освобождение выделенных ресурсов
  delete[] clients;
  delete[] client_ids;
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&room_available);

  std::cout << "Все клиенты обслужены. Программа завершается.\n";

  return 0;
}
