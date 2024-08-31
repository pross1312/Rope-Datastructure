#pragma once

#include <cassert>
#include <sstream>
#include <memory>
#include <list>
#include <cstring>
#include <stack>
#include <vector>
#include <string>
using std::make_shared;
using std::shared_ptr;
using std::string_view;
using std::vector;
using std::stack;

struct Rope {
    struct Node {
        size_t height;
        size_t length;
        size_t weight;
        char *data;
        shared_ptr<Node> left, right;

        Node(string_view str): Node((char*)str.data(), str.size()) {}
        Node(char *data, size_t length): height(1), length(length), weight(length), data(data), left(nullptr), right(nullptr) {}
        Node(shared_ptr<Node> left, shared_ptr<Node> right): height(left->height + 1), length(left->length), weight(left->length), data(nullptr), left(left), right(right) {
            if (right != nullptr) {
                height = std::max(height, right->height + 1);
                length += right->length;
            }
        }

        inline bool is_leave() { return data != nullptr; }

        inline bool is_balanced() {
            int lh = left == nullptr ? 0 : left->height;
            int rh = right == nullptr ? 0 : right->height;
            bool left_balanced = left == nullptr ? true : left->is_balanced();
            bool right_balanced = right == nullptr ? true : right->is_balanced();

            if (std::abs(lh - rh) <= 1 && left_balanced && right_balanced) return true;

            return false;
        }

        inline void split(size_t idx, vector<shared_ptr<Node>> &orphans) {
            if (is_leave()) {
                // if (idx == 0 || idx >= length) {
                //     printf("%zu %zu %zu %p %p\n", idx, length, weight, left.get(), right.get());
                //     assert(idx != 0 && idx < length);
                // }
                shared_ptr<Node> n_right = make_shared<Node>(data + idx, length - idx);
                orphans.push_back(n_right);
                length = idx;
                weight = idx;
            } else if (idx < weight) {
                // if (idx == left->weight) {
                //     printf("%zu %zu %zu %zu %p %p\n", idx, length, weight, height, left.get(), right.get());
                // }
                left->split(idx, orphans);
                if (right != nullptr) orphans.push_back(right);
                right = nullptr;
                weight = left->length;
                length = left->length;
                height = left->height+1;
            } else if (idx == weight) {
                assert(right != nullptr);
                orphans.push_back(right);
                right = nullptr;
                weight = left->length;
                length = left->length;
                height = left->height + 1;
            } else {
                right->split(idx-weight, orphans);
                height = std::max(left->height, right->height) + 1;
                length = left->length + right->length;
                weight = left->length;
            }
        }
    };

    struct LeaveIterator {
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Node;
            using pointer = shared_ptr<Node>;
            using reference = Node&;
            pointer operator*() const { return m_ptr; }
            pointer operator->() { return m_ptr; }

            inline void next() {
                if (stck.size() == 0) {
                    m_ptr = nullptr;
                } else {
                    pointer node = stck.top();
                    stck.pop();
                    node = node->right;
                    while (node != nullptr && !node->is_leave()) {
                        stck.push(node);
                        node = node->left;
                    }
                    m_ptr = node;
                }
            }

            Iterator operator++() {
                do {
                    next();
                } while (stck.size() > 0 && m_ptr == nullptr);
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
            friend bool operator!= (const Iterator& a, const Iterator& b) { return !(a == b); };

        private:
            friend LeaveIterator;
            Iterator(stack<pointer> &stck, pointer ptr): stck(stck), m_ptr(ptr) {}
            stack<pointer> &stck;
            pointer m_ptr;
        };

        LeaveIterator(shared_ptr<Node> root): stck() {
            while (root != nullptr) {
                stck.push(root);
                root = root->left;
            }
        }

        inline Iterator begin() {
            shared_ptr<Node> top = stck.top();
            Iterator result(stck, top);
            stck.pop();
            top = top->right;
            while (top != nullptr) {
                stck.push(top);
                top = top->left;
            }
            return result;
        }

        inline Iterator end() { return Iterator(stck, nullptr); }

    private:
        stack<shared_ptr<Node>> stck;
    };

    Rope(): root(nullptr) {}
    Rope(shared_ptr<Node> root): root(root) {}
    Rope(string_view str): Rope(str.data(), str.size()) {}
    Rope(const char *str, size_t len) {
        char *data = alloc(len);
        memcpy(data, str, len);
        root = make_shared<Node>(data, len);
    }
    Rope(const char *str): Rope(str, strlen(str)) {}
    ~Rope() {
        for (const char* ptr : to_delete) {
            delete[] ptr;
        }
    }

    inline size_t length() { return root->length; }
    inline char operator[](size_t idx) {
        assert(idx < root->length && "Index out of range");
        shared_ptr<Node> ptr = root;
        while (!ptr->is_leave()) {
            if (idx < ptr->weight) ptr = ptr->left;
            else {
                idx -= ptr->weight;
                ptr = ptr->right;
            }
        }
        return ptr->data[idx];
    }

    inline void concat(shared_ptr<Node> that_root) {
        this->root = make_shared<Node>(this->root, that_root);
        // rebalance();
    }

    inline shared_ptr<Node> split(size_t idx) {
        vector<shared_ptr<Node>> orphans;
        root->split(idx, orphans);
        shared_ptr<Node> root = merge(orphans, 0, orphans.size());
        return root;
    }

    inline void rebalance() { // this make rope every operation really slow -_-, not sure it's needed
        if (!root->is_balanced()) {
            vector<shared_ptr<Node>> leaves = this->leaves();
            root = merge(leaves, 0, leaves.size());
        }
    }

   inline void erase(size_t idx, size_t len) {
       assert(idx < this->root->length && "Start index out of range");
       assert(idx + len <= this->root->length && "Out of bound");
       if (len == 0) return;
       if (idx == 0) {
           shared_ptr<Node> right_part = split(len);
           this->root = right_part;
       } else {
           shared_ptr<Node> right_part = split(len);
           split(idx);
           concat(right_part);
       }
   }

    inline void insert(size_t idx, const char *str) {
        insert(idx, string_view(str, strlen(str)));
    }
    inline void insert(size_t idx, string_view str) {
        char *data = alloc(str.size());
        memcpy(data, str.data(), str.size());
        shared_ptr<Node> new_leave = make_shared<Node>(data, str.size());
        if (root == nullptr) {
            root = new_leave;
            return;
        }
        if (idx == 0) root = make_shared<Node>(new_leave, root);
        else if (idx == length()) root = make_shared<Node>(root, new_leave);
        else {
            shared_ptr<Node> right_part = split(idx);
            root = make_shared<Node>(root, new_leave);
            concat(right_part);
            return;
        }
        // rebalance();
    }


    inline std::string str() { 
        std::stringstream ss;
        for (shared_ptr<Node> ptr : LeaveIterator(root)) {
            ss << string_view(ptr->data, ptr->length);
        }
        return ss.str();
    }
    inline LeaveIterator leaves_iter() { return LeaveIterator(root); }
    inline vector<shared_ptr<Node>> leaves() {
        vector<shared_ptr<Node>> nodes;
        for (shared_ptr<Node> ptr : LeaveIterator(root)) {
            nodes.push_back(ptr);
        }
        return nodes;
    }

    static shared_ptr<Node> merge(const vector<shared_ptr<Node>> &leaves, size_t start, size_t end) {
        if (end <= start) return nullptr;

        size_t range = end - start;
        if (range == 1) return leaves[start];
        if (range == 2) return make_shared<Node>(leaves[start], leaves[start + 1]);

        size_t mid = start + (range / 2);
        return make_shared<Node>(merge(leaves, start, mid), merge(leaves, mid, end));
    }


public:
    shared_ptr<Node> root;

private:
    std::list<char*> to_delete;
    char* alloc(size_t size) {
        char *result = new char[size];
        assert(result != nullptr && "Buy more RAM!!!");
        to_delete.push_back(result);
        return result;
    }
};
