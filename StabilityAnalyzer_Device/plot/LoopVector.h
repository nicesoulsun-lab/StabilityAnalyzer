#ifndef LOOPVECTOR_H
#define LOOPVECTOR_H

#include <QObject>
#include <QVector>
#include <QDebug>

/* 循环队列 : 开辟一段内存进行数据存储，超出长度后，覆盖之前的数据*/

template<class T>
class LoopVector
{
public:
	LoopVector() {}

	LoopVector(int size) {
		reserve(size);
	}
	
    LoopVector (const LoopVector &lv){
        m_maxSize = lv.m_maxSize;
        reserve(m_maxSize);
        m_data = lv.m_data;
        m_size = lv.m_size;
        m_pos = lv.m_pos;

    }
    ~LoopVector() {}

	LoopVector &operator=(const LoopVector &lv) {
		m_data = lv.m_data;
		m_maxSize = lv.m_maxSize;
        reserve(m_maxSize);
		m_size = lv.m_size;
		m_pos = lv.m_pos;
		return *this;
	}

	T &operator[](const T & index) {
		if (index>m_size - 1) {
            qDebug() <<__FUNCTION__<< "LoopVector out of range";
            return m_data[m_size-1];
		}
		int realIndex = (index + m_pos) % m_maxSize;
		return m_data[realIndex];
	}

	friend QDebug operator<<(QDebug debug, const LoopVector &lv) {
		debug << "LoopVector(";
		for (int i = 0; i<lv.size() - 1; i++) {
			debug << lv.at(i) << ",";
		}
		if (!lv.isEmpty()) {
			debug << lv.at(lv.size() - 1);
		}
		debug << ")";
		return debug;
	}

	bool isEmpty() const {
		return m_size == 0;
	}

	void reserve(uint size) {
		m_maxSize = size;
		m_data.reserve(size);
		m_data.resize(size);
	}

    T atLast() const {
        return at(m_size-1);
    }
    T at(int index) const {
		if (index>m_size - 1) {
            qDebug() <<__FUNCTION__<< "LoopVector out of range";
            return m_data.at(m_size - 1);
		}
		int realIndex = (index + m_pos) % m_maxSize;
		return m_data.at(realIndex);
	}

	void clear() {
		m_size = 0;
	}

    void push_back(const T &val) {
		m_size++;
		if (m_size>m_maxSize) {
			m_pos++;
			m_pos %= m_maxSize;
			m_size = m_maxSize;
		}
		int realIndex = (m_size + m_pos - 1) % m_maxSize;
		m_data[realIndex] = val;
	}

    void pop_back() {
		m_size--;
		if (m_size<0) {
			m_size = 0;
		}
	}

    void push_front(const T &val) {
		m_size++;
		m_pos--;
		m_pos = (m_maxSize + m_pos) % m_maxSize;
		if (m_size>m_maxSize) {
			m_size = m_maxSize;
		}
		m_data[m_pos] = val;
	}

    void pop_front() {
		m_size--;
		if (m_size<0) {
			m_size = 0;
		}
		else {
			m_pos++;
			m_pos %= m_maxSize;
		}
	}

	int size() const {
		return m_size;
	}

    void remove(int index) {
		if (index<m_size&&index>0) {
			for (int i = index; i< m_size - 1; i++) {
				(*this)[i] = (*this)[i + 1];
			}
			m_size--;
		}
	}


    void insert(int i, const T &value){
        if(i>m_size){
            return;
        }

        m_size++;
        if (m_size>m_maxSize) {
            m_pos++;
            m_pos %= m_maxSize;
            m_size = m_maxSize;
        }
        int realIndex = (i + m_pos - 1) % m_maxSize;
        m_data.insert(realIndex,value);
    }



private:
	int m_maxSize = 1000;
	int m_size = 0;
	int m_pos = 0;
	QVector<T> m_data;
};

#endif
