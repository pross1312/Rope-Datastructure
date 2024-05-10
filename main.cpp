#include <string>
#include <iostream>
#include <optional>
#include <sstream>
#include <cassert>
#include <vector>
#include <stack>
#include <memory>
using namespace std;

struct Rope {
    struct Node {
        using NodePtr = shared_ptr<Node>;
        size_t weight;
        size_t height;
        size_t length;
        NodePtr left, right;
        optional<string> data;

        // construct non-leave node
        Node(NodePtr left, NodePtr right):
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

        inline void split(size_t idx, vector<NodePtr> &orphans) {
            if (is_leave()) {
                assert(idx != 0 && idx < length);
                NodePtr n_right = make_shared<Node>(data.value().substr(idx));
                orphans.push_back(n_right);
                left = make_shared<Node>(data.value().substr(0, idx));
                data = nullopt;
            } else if (idx < weight) {
                left->split(idx, orphans);
                if (right != nullptr) orphans.push_back(right);
                right = nullptr;
            } else if (idx == weight) {
                assert(right != nullptr);
                orphans.push_back(right);
                right = nullptr;
            } else {
                right->split(idx-weight, orphans);
            }
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
    using NodePtr = shared_ptr<Node>;

    struct LeaveIterator {
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Rope::Node;
            using pointer = NodePtr;
            using reference = Rope::Node&;
            reference operator*() const { return *m_ptr; }
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

            Iterator& operator++() {
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

        LeaveIterator(NodePtr root): stck() {
            while (root != nullptr) {
                stck.push(root);
                root = root->left;
            }
        }

        inline Iterator begin() {
            NodePtr top = stck.top();
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
        stack<NodePtr> stck;
    };

    Rope(NodePtr root): root(root) { rebalance(); }

    inline char operator[](size_t idx) {
        assert(idx < root->length && "Out of range");
        return root->at(idx);
    }

    inline size_t length() { return root->length; }

    inline void rebalance() {
        if (!root->is_balanced()) {
            vector<NodePtr> leaves = this->leaves();
            root = merge(leaves, 0, leaves.size());
        }
    }

    inline void concat(const Rope &that) {
        this->root = make_shared<Node>(this->root, that.root);
        rebalance();
    }

    inline Rope split(size_t idx) {
        vector<NodePtr> orphans;
        root->split(idx, orphans);
        NodePtr root = merge(orphans, 0, orphans.size());
        rebalance();
        Rope result(root);
        result.rebalance();
        return result;
    }

    inline void prepend(const string &s) {
        NodePtr ptr = root;
        while (ptr != nullptr) {
            if (ptr->is_leave()) {
                ptr->left = make_shared<Node>(s);
                ptr->right = make_shared<Node>(ptr->data.value());
                ptr->data = nullopt;
                break;
            } else {
                assert((ptr->left != nullptr || ptr->right != nullptr) && "Invalid non-leave node");
                if (ptr->left != nullptr) ptr = ptr->left;
                else ptr = ptr->right;
            }
        }
    }

    inline void append(const string &s) {
        NodePtr ptr = root;
        while (ptr != nullptr) {
            if (ptr->is_leave()) {
                ptr->left = make_shared<Node>(ptr->data.value());
                ptr->right = make_shared<Node>(s);
                ptr->data = nullopt;
                break;
            } else {
                assert((ptr->left != nullptr || ptr->right != nullptr) && "Invalid non-leave node");
                if (ptr->right != nullptr) ptr = ptr->right;
                else ptr = ptr->left;
            }
        }
    }

    inline void insert(size_t idx, const string &s) {
        if (idx == 0) prepend(s);
        else if (idx == length()) append(s);
        else {
            Rope right_part = split(idx);
            append(s);
            concat(right_part);
        }
        rebalance();
    }

    inline string to_str() {
        stringstream ss;
        for (const Node &ptr : leaves_iter()) ss << ptr.data.value();
        return ss.str();
    }

    inline LeaveIterator leaves_iter() { return LeaveIterator(root); }
    inline vector<NodePtr> leaves() {
        vector<NodePtr> nodes;
        for (const Node &ptr : LeaveIterator(root)) {
            nodes.push_back(make_shared<Node>(ptr.data.value()));
        }
        return nodes;
    }

    NodePtr root;

private:
    static NodePtr merge(const vector<NodePtr> &leaves, size_t start, size_t end) {
        if (end <= start) return nullptr;

        size_t range = end - start;
        if (range == 1) return leaves[start];
        if (range == 2) return make_shared<Node>(leaves[start], leaves[start + 1]);

        size_t mid = start + (range / 2);
        return make_shared<Node>(merge(leaves, start, mid), merge(leaves, mid, end));
    }
};



int main() {
    Rope::NodePtr n1 = make_shared<Rope::Node>("Hello ");
    Rope::NodePtr n2 = make_shared<Rope::Node>("world");
    Rope::NodePtr p1_2 = make_shared<Rope::Node>(n1, n2);

    Rope::NodePtr n3 = make_shared<Rope::Node>(" ,How ");
    Rope::NodePtr n4 = make_shared<Rope::Node>("are yo");
    Rope::NodePtr p3_4 = make_shared<Rope::Node>(n3, n4);

    Rope::NodePtr pp = make_shared<Rope::Node>(p1_2, p3_4);

    Rope::NodePtr n5 = make_shared<Rope::Node>("u today");
    Rope::NodePtr p5 = make_shared<Rope::Node>(n5, nullptr);

    Rope::NodePtr root = make_shared<Rope::Node>(pp, p5);

    Rope rope(root);
    cout << "\"" << rope.to_str() << "\"\n";
    rope.insert(0, "Just checking, ");
    cout << "\"" << rope.to_str() << "\"\n";
    rope.insert(rope.length()-6, ", Just checking");
    cout << "\"" << rope.to_str() << "\"\n";
    return 0;
}
