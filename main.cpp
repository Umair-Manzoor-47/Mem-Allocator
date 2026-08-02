#include <iostream>
#include <memory/linear_allocator.h>

class Shape {
public:
    virtual void draw() const = 0;
    virtual ~Shape() = default;
};

class Circle : public Shape {
public:
    void draw() const override { std::cout << "CIRCLE created.\n"; }
};

class Square : public Shape {
public:
    void draw() const override { std::cout << "SQUARE created.\n"; }
};

int main()
{
    LinearAllocator allocator(1024);

    void* circleMem = allocator.allocate(sizeof(Circle), alignof(Circle));

    Shape* circle = new (circleMem) Circle();
    circle->draw();

    void* squareMem = allocator.allocate(sizeof(Square), alignof(Square));
    Shape* square = new (squareMem) Square();
    square->draw();

    std::cout << "Bytes used: " << allocator.used()
              << " / " << allocator.capacity() << "\n";

    circle->~Shape();
    square->~Shape();

    allocator.reset();
    std::cout << "After reset, bytes used: " << allocator.used() << "\n";

    return 0;
}