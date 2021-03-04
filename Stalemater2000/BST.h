#pragma once

template<typename T>
struct BST
{
	Node<T> root;

	void Insert(T item);
	bool Contains(T item);
};

template<typename T>
struct Node
{
	T value;
	Node* left = nullptr;
	Node* right = nullptr;

	Node(T value);
	
};
