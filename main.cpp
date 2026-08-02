#include <memory/linear_allocator.h>
#include <demo/shape.h>

int main()
{
    LinearAllocator allocator(1024);

    void* circleMem = allocator.allocate(sizeof(Circle), alignof(Circle));
    Shape* circle = new (circleMem) Circle();
    circle->draw();

    void* squareMem = allocator.allocate(sizeof(Square), alignof(Square));
    Shape* square = new (squareMem) Square();
    square->draw();

    circle->~Shape();
    square->~Shape();
    allocator.reset();

    return 0;
}