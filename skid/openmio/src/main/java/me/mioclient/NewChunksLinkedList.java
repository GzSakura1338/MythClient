package me.mioclient;

import java.util.LinkedList;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksLinkedList.class */
public final class NewChunksLinkedList<E> extends LinkedList<E> {
    public final int num;

    @Override // java.util.LinkedList, java.util.AbstractList, java.util.AbstractCollection, java.util.Collection, java.util.List, java.util.Deque, java.util.Queue
    public boolean add(E e) {
        if (size() >= this.num) {
            removeFirst();
        }
        if (contains(e)) {
            return false;
        }
        return super.add(e);
    }

    public NewChunksLinkedList(int i) {
        this.num = i;
    }
}
