#pragma once

#include <stdexcept>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <string>
#include <new>

template <typename T>
class DynamicArray {
private:
    T* data;
    size_t _size;
    size_t _capacity;

    // Увеличение capacity
    void grow_capacity() {
        size_t new_capacity = (_capacity == 0) ? 1 : _capacity * 2;
        reserve(new_capacity);
    }

public:

    // Конструктор по умолчанию
    DynamicArray() : data(nullptr), _size(0), _capacity(0) {}

    // Конструктор с начальным размером
    explicit DynamicArray(size_t count) : data(nullptr), _size(count), _capacity(count) {
        if (count > 0) {
            data = new T[count]();  // Value-initialization
        }
    }

    // Конструктор с начальным размером и значением
    DynamicArray(size_t count, const T& value) : data(nullptr), _size(count), _capacity(count) {
        if (count > 0) {
            data = new T[count];
            for (size_t i = 0; i < count; ++i) {
                data[i] = value;
            }
        }
    }

    // Конструктор от initializer_list
    DynamicArray(std::initializer_list<T> init) : data(nullptr), _size(init.size()), _capacity(init.size()) {
        if (_size > 0) {
            data = new T[_size];
            size_t i = 0;
            for (const T& item : init) {
                data[i++] = item;
            }
        }
    }

    // Копирующий конструктор
    DynamicArray(const DynamicArray& other) : data(nullptr), _size(other._size), _capacity(other._capacity) {
        if (_capacity > 0) {
            data = new T[_capacity];
            for (size_t i = 0; i < _size; ++i) {
                data[i] = other.data[i];
            }
        }
    }

    // Перемещающий конструктор
    DynamicArray(DynamicArray&& other) noexcept 
        : data(other.data), _size(other._size), _capacity(other._capacity) {
        other.data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    // Деструктор
    ~DynamicArray() {
        delete[] data;
    }

    // Копирующий оператор присваивания
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            T* new_data = nullptr;
            if (other._capacity > 0) {
                new_data = new T[other._capacity];
                for (size_t i = 0; i < other._size; ++i) {
                    new_data[i] = other.data[i];
                }
            }
            
            delete[] data;
            data = new_data;
            _size = other._size;
            _capacity = other._capacity;
        }
        return *this;
    }

    // Перемещающий оператор присваивания
    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            _size = other._size;
            _capacity = other._capacity;
            
            other.data = nullptr;
            other._size = 0;
            other._capacity = 0;
        }
        return *this;
    }

    // ДОСТУП К ЭЛЕМЕНТАМ

    // Доступ с проверкой границ
    T& at(size_t index) {
        if (index >= _size) {
            throw std::out_of_range("DynamicArray::at: индекс выходит за пределы (index=" + 
                                   std::to_string(index) + ", size=" + std::to_string(_size) + ")");
        }
        return data[index];
    }

    const T& at(size_t index) const {
        if (index >= _size) {
            throw std::out_of_range("DynamicArray::at: индекс выходит за пределы (index=" + 
                                   std::to_string(index) + ", size=" + std::to_string(_size) + ")");
        }
        return data[index];
    }

    // Доступ без проверки границ
    T& operator[](size_t index) {
        return data[index];
    }

    const T& operator[](size_t index) const {
        return data[index];
    }

    // Первый и последний элементы
    T& front() {
        if (_size == 0) throw std::out_of_range("DynamicArray::front: массив пуст");
        return data[0];
    }

    const T& front() const {
        if (_size == 0) throw std::out_of_range("DynamicArray::front: массив пуст");
        return data[0];
    }

    T& back() {
        if (_size == 0) throw std::out_of_range("DynamicArray::back: массив пуст");
        return data[_size - 1];
    }

    const T& back() const {
        if (_size == 0) throw std::out_of_range("DynamicArray::back: массив пуст");
        return data[_size - 1];
    }

    // Сырой указатель на данные
    T* data_ptr() noexcept { return data; }
    const T* data_ptr() const noexcept { return data; }

    // ИТЕРАТОРЫ (упрощенные)

    T* begin() noexcept { return data; }
    const T* begin() const noexcept { return data; }

    T* end() noexcept { return data + _size; }
    const T* end() const noexcept { return data + _size; }

    // РАЗМЕР И ЕМКОСТЬ

    size_t size() const noexcept { return _size; }
    size_t capacity() const noexcept { return _capacity; }
    bool empty() const noexcept { return _size == 0; }

    // МОДИФИКАЦИЯ

    // Добавление элемента в конец
    void push_back(const T& value) {
        if (_size >= _capacity) {
            grow_capacity();
        }
        data[_size++] = value;
    }

    void push_back(T&& value) {
        if (_size >= _capacity) {
            grow_capacity();
        }
        data[_size++] = std::move(value);
    }

    // Конструирование элемента на месте
    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (_size >= _capacity) {
            grow_capacity();
        }
        new (&data[_size]) T(std::forward<Args>(args)...);
        ++_size;
    }

    // Удаление последнего элемента
    void pop_back() {
        if (_size == 0) {
            throw std::out_of_range("DynamicArray::pop_back: массив пуст");
        }
        --_size;
    }

    // Очистка массива
    void clear() noexcept {
        _size = 0;
    }

    // Изменение размера
    void resize(size_t new_size) {
        if (new_size > _capacity) {
            reserve(new_size);
        }
        _size = new_size;
    }

    void resize(size_t new_size, const T& value) {
        if (new_size > _capacity) {
            reserve(new_size);
        }
        for (size_t i = _size; i < new_size; ++i) {
            data[i] = value;
        }
        _size = new_size;
    }

    // Резервирование памяти
    void reserve(size_t new_capacity) {
        if (new_capacity <= _capacity) return;

        T* new_data = new T[new_capacity];
        
        // Копируем существующие элементы
        for (size_t i = 0; i < _size; ++i) {
            new_data[i] = std::move(data[i]);
        }

        delete[] data;
        data = new_data;
        _capacity = new_capacity;
    }

    // Удаление элемента по индексу
    void erase(size_t index) {
        if (index >= _size) {
            throw std::out_of_range("DynamicArray::erase: индекс выходит за пределы");
        }

        // Сдвигаем элементы влево
        for (size_t i = index; i < _size - 1; ++i) {
            data[i] = std::move(data[i + 1]);
        }
        --_size;
    }

    // Вставка элемента по индексу
    void insert(size_t index, const T& value) {
        if (index > _size) {
            throw std::out_of_range("DynamicArray::insert: индекс выходит за пределы");
        }

        if (_size >= _capacity) {
            grow_capacity();
        }

        // Сдвигаем элементы вправо
        for (size_t i = _size; i > index; --i) {
            data[i] = std::move(data[i - 1]);
        }

        data[index] = value;
        ++_size;
    }

    // СРАВНЕНИЕ

    bool operator==(const DynamicArray& other) const {
        if (_size != other._size) return false;
        for (size_t i = 0; i < _size; ++i) {
            if (!(data[i] == other.data[i])) return false;
        }
        return true;
    }

    bool operator!=(const DynamicArray& other) const {
        return !(*this == other);
    }
};