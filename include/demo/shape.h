#pragma once
#include <iostream>
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