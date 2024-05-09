#include <cstdio>
#include <string>
#include <optional>
#include <cassert>
#include <vector>
#include <stack>
#include <memory>
using namespace std;

struct Rope {
    struct Node {
        size_t weight;
        size_t height;
        size_t length;
        shared_ptr<Node>left, right;
        optional<string> data;

        // construct non-leave node
        Node(shared_ptr<Node> left, shared_ptr<Node> right):
            weight(left->length), height(left->height+1), length(left->length), left(left), right(right), data() {
            assert(left != nullptr && "A non-leave node must have at least a left child");
            if (right != nullptr) {
                height = std::max(height, right->height + 1);
                length += right->length;
            }
        }

        // construct leave node
        Node(string data): weight(data.size()), height(1), length(data.size()), left(nullptr), right(nullptr), data(data) {}

        inline char at(size_t idx) {
            if (is_leave()) return data.value()[idx];
            if (idx >= weight) return right->at(idx - weight);
            return left->at(idx);
        }

        inline bool is_balanced() {
            int lh = left == nullptr ? 0 : left->height;
            int rh = right == nullptr ? 0 : right->height;
            bool left_balanced = left == nullptr ? true : left->is_balanced();
            bool right_balanced = right == nullptr ? true : right->is_balanced();

            if (std::abs(lh - rh) <= 1 && left_balanced && right_balanced) return true;

            return false;
        }

        inline bool is_leave() { return data != nullopt; }
    };

    struct LeaveIterator {
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Rope::Node;
            using pointer = shared_ptr<Rope::Node>;
            using reference = Rope::Node&;
            reference operator*() const { return *m_ptr; }
            pointer operator->() { return m_ptr; }


            Iterator& operator++() {
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
                    // assert(node != nullptr && "Can't have hanging non-leave node");
                    m_ptr= node;
                }
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator== (const Iterator& a, const Iterator& b) {
                return a.m_ptr == b.m_ptr;
            };
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

        Iterator begin() {
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

        Iterator end() {
            return Iterator(stck, nullptr);
        }

    private:
        stack<shared_ptr<Node>> stck;
    };

    Rope(shared_ptr<Node> root): root(root) {}

    inline char operator[](size_t idx) {
        assert(idx < root->length && "Out of range");
        return root->at(idx);
    }

    inline size_t length() { return root->length; }

    inline void rebalance() {
        if (!root->is_balanced()) {
            vector<shared_ptr<Node>> leaves = this->leaves();
            root = merge(leaves, 0, leaves.size());
        }
    }

    inline void concat(const Rope &that) {
        this->root = make_shared<Node>(this->root, that.root);
        rebalance();
    }


    inline LeaveIterator leaves_iter() { return LeaveIterator(root); }
    inline vector<shared_ptr<Node>> leaves() {
        vector<shared_ptr<Node>> nodes;
        for (const Node &ptr : LeaveIterator(root)) {
            nodes.push_back(make_shared<Node>(ptr.data.value()));
        }
        return nodes;
    }

    shared_ptr<Node> root;

private:
    static shared_ptr<Node> merge(const vector<shared_ptr<Node>> &leaves, size_t start, size_t end) {
        if (end <= start) return nullptr;

        size_t range = end - start;
        if (range == 1) return leaves[start];
        if (range == 2) return make_shared<Node>(leaves[start], leaves[start + 1]);

        size_t mid = start + (range / 2);
        return make_shared<Node>(merge(leaves, start, mid), merge(leaves, mid, end));
    }
};



int main() {
    shared_ptr<Rope::Node>n1 = make_shared<Rope::Node>("Hello ");
    shared_ptr<Rope::Node>n2 = make_shared<Rope::Node>("world");
    shared_ptr<Rope::Node>p1_2 = make_shared<Rope::Node>(n1, n2);

    shared_ptr<Rope::Node>n3 = make_shared<Rope::Node>(" ,How ");
    shared_ptr<Rope::Node>n4 = make_shared<Rope::Node>("are yo");
    shared_ptr<Rope::Node>p3_4 = make_shared<Rope::Node>(n3, n4);

    shared_ptr<Rope::Node>pp = make_shared<Rope::Node>(p1_2, p3_4);

    shared_ptr<Rope::Node>n5 = make_shared<Rope::Node>("u today");
    shared_ptr<Rope::Node>p5 = make_shared<Rope::Node>(n5, nullptr);

    shared_ptr<Rope::Node>root = make_shared<Rope::Node>(pp, p5);

    Rope rope(root);
    rope.rebalance();

    for (const Rope::Node &ptr : rope.leaves_iter()) {
        printf("%.*s", (int)ptr.weight, ptr.data.value().data());
    }
    printf("\n");
    for (size_t i = 0; i < rope.length(); i++) {
        printf("Char at %zu: %c\n", i, rope[i]);
    }
    return 0;
}
