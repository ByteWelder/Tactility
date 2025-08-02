#pragma once

template <typename DataType>
class Dequeue {

    struct Node {
        DataType data;
        Node* next;
        Node* previous;

        Node(DataType data, Node* next, Node* previous):
            data(data),
            next(next),
            previous(previous)
        {}
    };

    int count = 0;
    Node* head = nullptr;
    Node* tail = nullptr;

public:

    void pushFront(DataType data) {
        auto* new_node = new Node(data, head, nullptr);

        if (head != nullptr) {
            head->previous = new_node;
        }

        if (tail == nullptr) {
            tail = new_node;
        }

        head = new_node;
        count++;
    }

    void pushBack(DataType data) {
        auto* new_node = new Node(data, nullptr, tail);

        if (head == nullptr) {
            head = new_node;
        }

        if (tail != nullptr) {
            tail->next = new_node;
        }

        tail = new_node;
        count++;
    }

    void popFront() {
        if (head != nullptr) {
            bool is_last_node = (head == tail);
            Node* node_to_delete = head;
            head = node_to_delete->next;
            if (is_last_node) {
                tail = nullptr;
            }
            delete node_to_delete;
            count--;
        }
    }

    void popBack() {
        if (tail != nullptr) {
            bool is_last_node = (head == tail);
            Node* node_to_delete = tail;
            tail = node_to_delete->previous;
            if (is_last_node) {
                head = nullptr;
            }
            delete node_to_delete;
            count--;
        }
    }

    DataType back() const {
        assert(tail != nullptr);
        return tail->data;
    }

    DataType front() const {
        assert(head != nullptr);
        return head->data;
    }

    bool empty() const {
        return head == nullptr;
    }

    int size() const { return count; }
};