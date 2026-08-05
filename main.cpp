#include <demo/allocator_demos.h>

int main()
{
    demo_linear_allocator();
    demo_tracking_allocator();
    demo_stack_allocator();
    return 0;
}