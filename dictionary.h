#ifndef DICTIONARY_H_INCLUDED
#define DICTIONARY_H_INCLUDED

#include <cstddef>
#include <cassert>
#include <utility>

template<typename K, typename V>
class Dictionary
{
    enum class Color { Black, Red };
    struct Node
    {
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        K key;
        V value;
        Color color;

        Node(const K& k, const V& v, Color c = Color::Red)
            : key(k)
            , value(v)
            , color(c)
        { }

        Node* clone() const { return new Node(key, value, color); }
    };

public:
    Dictionary() = default;

    Dictionary(const Dictionary& other)
    {
        root = copy(other.root, nullptr);
        nodes_count = other.nodes_count;
    }

    ~Dictionary()
    {
        erase(root);
    }

    Dictionary& operator=(const Dictionary& other)
    {
        Dictionary tmp(other);
        std::swap(root, tmp.root);
        std::swap(nodes_count, tmp.nodes_count);
        return *this;
    }

    V& operator[](const K& key)
    {
        Node* result = nullptr;
        Node* node_parent = nullptr;
        result = find(key, &node_parent);
        if (!result || result->key != key)
        {
            auto node = new Node(key, V{});
            result = insert(node, node_parent);
        }
        return result->value;
    }

    const V& operator[](const K& key) const
    {
        static V default_value;
        auto result = find(key);
        return result ? result->value : default_value;
    }

    void put(const K& key, const V& value)
    {
        Node* node_parent = nullptr;

        auto tmp = find(key, &node_parent);
        if (tmp && tmp->key == key)
        {
            tmp->value = value;
        }
        else
        {
            auto node = new Node(key, value);
            insert(node, node_parent);
        }
    }

    void remove(const K& key)
    {
        Node* node = find(key);
        if (node)
        {
            Node* successor = node;
            Node* parent = nullptr;
            Node* child = nullptr;

            if (!successor->left)
                child = successor->right;
            else if (!successor->right)
                child = successor->left;
            else
            {
                successor = successor->right;
                while (successor->left)
                    successor = successor->left;
                child = successor->right;
            }

            if (successor != node)
            {
                node->left->parent = successor;
                successor->left = node->left;
                if (successor != node->right)
                {
                    parent = successor->parent;
                    if (child)
                        child->parent = successor->parent;
                    successor->parent->left = child;
                    successor->right = node->right;
                    successor->right->parent = successor;
                }
                else
                    parent = successor;

                if (root == node)
                    root = successor;
                else if (node->parent->left == node)
                    node->parent->left = successor;
                else
                    node->parent->right = successor;

                successor->parent = node->parent;
                std::swap(successor->color, node->color);
                successor = node;
            }
            else
            {
                parent = successor->parent;
                if (child)
                    child->parent = successor->parent;

                if (root == node)
                    root = child;
                else if (node->parent->left == node)
                    node->parent->left = child;
                else
                    node->parent->right = child;
            }

            if (successor->color == Color::Black)
                rebalanceDeletion(child, parent);
            delete node;
            --nodes_count;
        }
    }

    bool contains(const K& key) const
    {
        auto result = find(key);
        return result && result->key == key;
    }

    size_t size() const
    {
        return nodes_count;
    }

    class Iterator;

    Iterator iterator()
    {
        return Iterator(this);
    }

private:
    Node* find(const K& key, Node** hint = nullptr) const
    {
        auto result = root;

        while (result)
        {
            if (hint)
                *hint = result;

            if (key < result->key)
                result = result->left;
            else if (key == result->key)
                break;
            else
                result = result->right;
        }
        return result;
    }

    Node* copy(Node* from, Node* parent)
    {
        Node* result = from->clone();
        result->parent = parent;
        try
        {
            if (from->right)
                result->right = copy(from->right, result);
            parent = result;
            from = from->left;

            while (from)
            {
                Node* node = from->clone();
                parent->left = node;
                node->parent = parent;
                if (from->right)
                    node->right = copy(from->right, node);
                parent = node;
                from = from->left;
            }
        }
        catch (...)
        {
            erase(result);
            throw;
        }
        return result;
    }

    Node* insert(Node* node, Node* parent)
    {
        node->parent = parent;
        if (!parent)
            root = node;
        else if (node->key < parent->key)
            parent->left = node;
        else
            parent->right = node;

        rebalanceInsertion(node);
        ++nodes_count;
        return node;
    }

    void erase(Node* node)
    {
        while (node)
        {
            erase(node->right);
            auto tmp = node->left;
            delete node;
            node = tmp;
        }
    }

    void rotateLeft(Node* node)
    {
        assert(node->right);

        auto subtree_new_root = node->right;
        node->right = subtree_new_root->left;

        if (subtree_new_root->left)
            subtree_new_root->left->parent = node;

        subtree_new_root->parent = node->parent;
        if (!node->parent)
            root = subtree_new_root;
        else if (node == node->parent->left)
            node->parent->left = subtree_new_root;
        else
            node->parent->right = subtree_new_root;

        subtree_new_root->left = node;
        node->parent = subtree_new_root;
    }

    void rotateRight(Node* node)
    {
        assert(node->left);

        auto subtree_new_root = node->left;
        node->left = subtree_new_root->right;

        if (subtree_new_root->right)
            subtree_new_root->right->parent = node;

        subtree_new_root->parent = node->parent;
        if (!node->parent)
            root = subtree_new_root;
        else if (node == node->parent->right)
            node->parent->right = subtree_new_root;
        else
            node->parent->left = subtree_new_root;

        subtree_new_root->right = node;
        node->parent = subtree_new_root;
    }

    void rebalanceInsertion(Node* node)
    {
        while (node != root && node->parent->color == Color::Red)
        {
            auto grandparent = node->parent->parent;
            if (node->parent == grandparent->left)
            {
                auto uncle = grandparent->right;
                if (uncle && uncle->color == Color::Red)
                {
                    node->parent->color = Color::Black;
                    uncle->color = Color::Black;
                    grandparent->color = Color::Red;
                    node = grandparent;
                }
                else
                {
                    if (node == node->parent->right)
                    {
                        node = node->parent;
                        rotateLeft(node);
                    }
                    node->parent->color = Color::Black;
                    grandparent->color = Color::Red;
                    rotateRight(grandparent);
                }
            }
            else
            {
                auto uncle = grandparent->left;
                if (uncle && uncle->color == Color::Red)
                {
                    node->parent->color = Color::Black;
                    uncle->color = Color::Black;
                    grandparent->color = Color::Red;
                    node = grandparent;
                }
                else
                {
                    if (node == node->parent->left)
                    {
                        node = node->parent;
                        rotateRight(node);
                    }
                    node->parent->color = Color::Black;
                    grandparent->color = Color::Red;
                    rotateLeft(grandparent);
                }
            }
        }
        root->color = Color::Black;
    }

    void rebalanceDeletion(Node* node, Node* parent)
    {
        while (node != root && (!node || node->color == Color::Black))
        {
            if (node == parent->left)
            {
                Node* sibling = parent->right;

                if (sibling->color == Color::Red)
                {
                    sibling->color = Color::Black;
                    parent->color = Color::Red;
                    rotateLeft(parent);
                    sibling = parent->right;
                }

                if ((!sibling->left || sibling->left->color == Color::Black)
                        && (!sibling->right || sibling->right->color == Color::Black))
                {
                    sibling->color = Color::Red;
                    node = parent;
                    parent = parent->parent;
                }
                else
                {
                    if (!sibling->right || sibling->right->color == Color::Black)
                    {
                        sibling->left->color = Color::Black;
                        sibling->color = Color::Red;
                        rotateRight(sibling);
                        sibling = parent->right;
                    }
                    sibling->color = parent->color;
                    parent->color = Color::Black;
                    if (sibling->right)
                        sibling->right->color = Color::Black;
                    rotateLeft(parent);
                    break;
                }
            }
            else
            {
                Node* sibling = parent->left;

                if (sibling->color == Color::Red)
                {
                    sibling->color = Color::Black;
                    parent->color = Color::Red;
                    rotateRight(parent);
                    sibling = parent->left;
                }

                if ((!sibling->right || sibling->right->color == Color::Black)
                        && (!sibling->left || sibling->left->color == Color::Black))
                {
                    sibling->color = Color::Red;
                    node = parent;
                    parent = parent->parent;
                }
                else
                {
                    if (!sibling->left || sibling->left->color == Color::Black)
                    {
                        sibling->right->color = Color::Black;
                        sibling->color = Color::Red;
                        rotateLeft(sibling);
                        sibling = parent->left;
                    }
                    sibling->color = parent->color;
                    parent->color = Color::Black;
                    if (sibling->left)
                        sibling->left->color = Color::Black;
                    rotateRight(parent);
                    break;
                }
            }
            if (node)
                node->color = Color::Black;
        }
    }

    Node* root = nullptr;
    size_t nodes_count = 0;
};

template<typename K, typename V>
class Dictionary<K, V>::Iterator
{
public:
    Iterator() = delete;

    const K& key() const
    {
        return ptr->key;
    }

    const V& get() const
    {
        return ptr->value;
    }

    void set(const V& value)
    {
        ptr->value = value;
    }

    void next()
    {
        if (ptr->right)
        {
            ptr = ptr->right;
            while (ptr->left)
                ptr = ptr->left;
        }
        else
        {
            Node* parent = ptr->parent;
            while (parent && ptr == parent->right)
            {
                ptr = parent;
                parent = parent->parent;
            }
            if (ptr->right != parent)
                ptr = parent;
        }
    }

    void prev()
    {
        if (ptr->color == Dictionary::Color::Red && ptr->parent->parent == ptr)
            ptr = ptr->right;
        else if (ptr->left)
        {
            Dictionary::Node* left = ptr->left;
            while (left->right)
                left = left->right;
            ptr = left;
        }
        else
        {
            Dictionary::Node* parent = ptr->parent;
            while (parent && ptr == parent->left)
            {
                ptr = parent;
                parent = parent->parent;
            }
            ptr = parent;
        }
    }

    bool hasNext() const
    {
        Iterator it(*this);
        if (ptr)
            it.next();
        return it.ptr != nullptr;
        //return ptr && (ptr->right || (ptr->parent && ptr->parent->right)
        //    || (ptr->parent->parent && ptr->parent->parent->right));
    }

    bool hasPrev() const
    {
        Iterator it(*this);
        if (ptr)
            it.prev();
        return it.ptr != nullptr;
        //return ptr && (ptr->left || (ptr->parent && ptr->parent->left)
        //            || (ptr->parent->parent && ptr->parent->parent->left));
    }

private:
    explicit Iterator(Dictionary* d)
        : dict(d)
        , ptr(d->root)
    {
        while (ptr && ptr->left)
            ptr = ptr->left;
    }

private:
    friend class Dictionary;

    Dictionary* dict;
    Dictionary::Node* ptr;
};

#endif // DICTIONARY_H_INCLUDED
