#ifndef DICTIONARY_H_INCLUDED
#define DICTIONARY_H_INCLUDED

#include <cstddef>
#include <cassert>
#include <utility>

// класс ассоциативного массива
// на остнове красно-черного дерева
template<typename K, typename V>
class Dictionary
{
    enum class Color { Black, Red };
    // класс узла дерева
    struct Node
    {
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        K key;
        V value;
        Color color;

        // новый узел - красный по умолчанию
        Node(const K& k, const V& v, Color c = Color::Red)
            : key(k)
            , value(v)
            , color(c)
        { }

        // копия узла
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

    // индексация
    V& operator[](const K& key)
    {
        Node* result = nullptr;
        Node* node_parent = nullptr;
        result = find(key, &node_parent);
        if (!result || result->key != key)
        {   // если элемента с ключом key не существует
            // создаем новый узел
            auto node = new Node(key, V{});
            // вставляем его
            result = insert(node, node_parent);
        }
        return result->value;
    }

    const V& operator[](const K& key) const
    {
        static V default_value;
        auto result = find(key);
        // если ключ найден, вернем его значение, иначе - значение по умолчанию
        return result ? result->value : default_value;
    }

    // добавляет пару в массива
    void put(const K& key, const V& value)
    {
        Node* node_parent = nullptr;

        auto tmp = find(key, &node_parent);
        if (tmp && tmp->key == key)
        { // если ключ найден, изменяем значение
            tmp->value = value;
        }
        else
        { // иначе вставляем новый узел
            auto node = new Node(key, value);
            insert(node, node_parent);
        }
    }

    // удалить элемент с указанным ключом
    void remove(const K& key)
    {
        Node* node = find(key);
        if (node) // если есть такой элемент
        { 
            Node* successor = node;
            Node* parent = nullptr;
            Node* child = nullptr;

            if (!successor->left)
                child = successor->right;
            else if (!successor->right)
                child = successor->left;
            else
            { // если узел имеет оба дочерних узла
                // находим узел с последующим значением
                // в отсортированной последовательности
                successor = successor->right;
                while (successor->left)
                    successor = successor->left;
                // и его потомка
                child = successor->right;
            }

            if (successor != node)
            { // если узел имеет оба дочерних узла, перенацеливаем узлы, исключая node из связи
                // перенацеливаем родителя левого потомка узла node
                node->left->parent = successor;
                // перенацеливаем левого потомка узла successor
                successor->left = node->left;
                if (successor != node->right)
                { // если последующий узел не является сразу правым потомком
                    parent = successor->parent;
                    // перенацеливаем потомка узла successor
                    if (child)
                        child->parent = successor->parent;
                    successor->parent->left = child;
                    // перенацеливаем правого потомка узла successor
                    successor->right = node->right;
                    // и родителя потомка
                    successor->right->parent = successor;
                }
                else
                    parent = successor;

                // перенацеливаем родителя node на successor
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
            else // удаляемый узел имеет не более 1 потомка
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

            if (successor->color == Color::Black) // если удалили черный узел
                // восстанавливаем красно-черное свойство (черную высоту)
                rebalanceDeletion(child, parent);
            delete node;
            --nodes_count;
        }
    }

    // существование элемента с зажанным ключом
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
    // поиск узла по заданному ключу, если найден - в hint предок узла
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

    // создает копию дерева с корнем в from,
    // устанавливая предка для корня в parent
    Node* copy(Node* from, Node* parent)
    {
        Node* result = from->clone(); // создаем копию узла
        result->parent = parent; // назначаем ему предка
        try
        {
            if (from->right) // полное копирование правого поддерева
                result->right = copy(from->right, result);
            parent = result;
            from = from->left;

            while (from)
            { // вглубь к самому левому потомку
                // создаем копию левого узла
                Node* node = from->clone();
                parent->left = node;
                node->parent = parent;
                if (from->right) // полное копирование правого поддерева левого потомка
                    node->right = copy(from->right, node);
                parent = node;
                from = from->left;  // к левому потомку
            }
        }
        catch (...)
        {
            erase(result);
            throw;
        }
        return result;
    }

    // вставка node потомком узла parent
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

    // левый поворот
    void rotateLeft(Node* node)
    {
        assert(node->right);
        // при повороте новым корнем поддерева будет правый потомок
        auto subtree_new_root = node->right;
        // правым потомком старого корня будет бывший левый потомок нового корня
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
        // левым потомком нового корня будет старый корень
        subtree_new_root->left = node;
        node->parent = subtree_new_root;
    }

    // правый поворот
    void rotateRight(Node* node)
    {
        assert(node->left);
        // новым корнем поддерева будет левый потомок
        auto subtree_new_root = node->left;
        // левым потомком старого корня будет бывший правый потомок нового корня
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
        // правым потомком нового корня будет старый корень
        subtree_new_root->right = node;
        node->parent = subtree_new_root;
    }

    // восстановление красно-черных свойств после вставки
    void rebalanceInsertion(Node* node)
    {
        while (node != root && node->parent->color == Color::Red)
        { // если нарушено красно-черное свойство (узел и предок - красные)
            auto grandparent = node->parent->parent; // предок родителя (дед)
            if (node->parent == grandparent->left)
            { // если предок - левый сын деда
                auto uncle = grandparent->right; // смежный с родительским узел ("дядя")
                if (uncle && uncle->color == Color::Red)
                { // если дядя тоже красный
                    node->parent->color = Color::Black;
                    uncle->color = Color::Black;
                    // задаем деду красный цвет, для восстановления свойств выше по дереву
                    grandparent->color = Color::Red;
                    node = grandparent;
                }
                else
                {  // дядя узла - черный, узел и его предок - красные
                    if (node == node->parent->right)
                    { // если узел - правый сын, совершаем левый поворот вокруг связи сын-предок
                        node = node->parent;
                        rotateLeft(node);
                    }
                    node->parent->color = Color::Black;
                    grandparent->color = Color::Red;
                    // совершаем правый поворот вокруг связи предок-дед
                    rotateRight(grandparent);
                }
            }
            else // если предок - правый сын деда
            {  // то же, что и выше, только вмето левого - правый и наоборот
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

    // восстановление красно-черных свойств после удаления
    void rebalanceDeletion(Node* node, Node* parent)
    {
        while (node != root && (!node || node->color == Color::Black))
        {
            if (node == parent->left)
            { // если узел левый потомок
                Node* sibling = parent->right; // смежный узел ("брат") узлу node

                if (sibling->color == Color::Red)
                { // если брат - красный, то все его потомки черные, можем обменять цвета
                    sibling->color = Color::Black;
                    parent->color = Color::Red;
                    rotateLeft(parent); // и сделать левый поворот вокруг связи предок-брат
                    sibling = parent->right; // новый (после поворота) брат
                }

                if ((!sibling->left || sibling->left->color == Color::Black)
                        && (!sibling->right || sibling->right->color == Color::Black))
                { // если оба потомка брата - черные, можем забрать окраску брата
                    sibling->color = Color::Red;
                    node = parent;
                    parent = parent->parent;
                }
                else
                { // не все потомки брата - черные
                    if (!sibling->right || sibling->right->color == Color::Black)
                    { // если правый потомок брата - черный
                        sibling->left->color = Color::Black; // красим левого потомка
                        sibling->color = Color::Red; // забираем окрас у брата
                        rotateRight(sibling); // и совершаем правый поворот
                        sibling = parent->right;
                    }
                    // выполняем обмен цветов
                    sibling->color = parent->color;
                    parent->color = Color::Black;
                    if (sibling->right)
                        sibling->right->color = Color::Black;
                    rotateLeft(parent); // и совершаем правый поворот
                    break;
                }
            }
            else // если узел правый потомок
            { // то же, что и выше, только вмето левого - правый и наоборот
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
