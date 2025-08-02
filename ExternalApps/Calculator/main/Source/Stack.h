#pragma once

#include "Dequeue.h"

template <typename DataType>
class Stack {

    Dequeue<DataType> dequeue;

public:

    void push(DataType data) { dequeue.pushFront(data); }

    void pop() { dequeue.popFront(); }

    DataType top() const { return dequeue.front(); }

    bool empty() const { return dequeue.empty(); }

    int size() const { return dequeue.size(); }
};
