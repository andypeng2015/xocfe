/*@
Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/
#ifndef __INT_HASH_H__
#define __INT_HASH_H__

namespace xcom {

//The class represents the intermediate data structure of the integer hash
//table.
//Note the Node allocates memory that is used internally in a given memory
//pool, thus there is no need to free anything when the Node is destructing.
//Since the class knows nothing about 'MappedObj', the member 'mapped' will
//require an initial value when constructing the class object.
template <typename IntType, class MappedObj>
class IntVal2Node {
    COPY_CONSTRUCTOR(IntVal2Node);
public:
    typedef TMap<IntType, IntVal2Node<IntType, MappedObj>*> NextSet;
    typedef TMapIter<IntType, IntVal2Node<IntType, MappedObj>*> NextSetIter;
    MappedObj mapped;
    NextSet next;
public:
    IntVal2Node(SMemPool * pool) : next(pool)
    { mapped = MappedObj(0); }
    ~IntVal2Node() {}
    void init(SMemPool * pool)
    {
        mapped = MappedObj(0);
        next.init(pool);
    }
    void destroy()
    {
        mapped = MappedObj(0);
        next.destroy();
    }

    //Count memory usage for current object.
    size_t count_mem() const
    { return next.count_mem() + (size_t)sizeof(mapped); }
};

//The class maps an integer set to a specific object.
template <typename IntType, class MappedObj>
class IntSetMap {
    COPY_CONSTRUCTOR(IntSetMap);
protected:
    SMemPool * m_pool;
    #ifdef _DEBUG_
    UINT m_num_node; //record the number of MD2Node in the tree.
    #endif
    typedef IntVal2Node<IntType, MappedObj> V2NMap;
    SMemPool * m_rbtn_pool; //pool to store RBTNType.
    V2NMap * m_root_val2node;
protected:
    inline V2NMap * allocV2NMap()
    {
        V2NMap * mn = (V2NMap*)smpoolMallocConstSize(sizeof(V2NMap), m_pool);
        ::memset((void*)mn, 0, sizeof(V2NMap));
        mn->init(m_rbtn_pool);
        #ifdef _DEBUG_
        m_num_node++;
        #endif
        return mn;
    }

    //The target dependent code to dump the content of user defined
    //MappedObject.
    virtual void dumpMappedObj(
        FILE * h, UINT indent, MappedObj const& mapped) const {}

    void dump_helper_mapped(
        FILE * h, MappedObj const& mapped, UINT indent, IntType ival) const
    {
        log(h, 0, "\n");
        CHAR const* fmt = getIntTypeFormat(false);
        log(h, indent, fmt, ival);
        dumpMappedObj(h, indent, mapped);
    }

    //dump_helper for IntVal2Node.
    void dump_helper(FILE * h, V2NMap const* mn, UINT indent) const
    {
        ASSERT0(mn);
        typename V2NMap::NextSetIter ti;
        Stack<V2NMap const*> mn_stack;
        Stack<UINT> indent_stack;
        Stack<IntType> ival_stack;
        mn_stack.push(mn);
        ival_stack.push(IntType(0));
        indent_stack.push(indent);
        for (; mn_stack.get_top() != nullptr;) {
            V2NMap const* mn2 = mn_stack.pop();
            IntType ival = ival_stack.pop();
            UINT ind = indent_stack.pop();
            ti.clean();
            dump_helper_mapped(h, mn2->mapped, ind, ival);
            V2NMap * nextmn = nullptr;
            IntType nival;
            for (nival = mn2->next.get_first(ti, &nextmn);
                 !ti.end(); nival = mn2->next.get_next(ti, &nextmn)) {
                ASSERT0(nextmn);
                mn_stack.push(nextmn);
                ival_stack.push(nival);
                indent_stack.push(ind + 2);
            }
        }
    }

    //Return the format string according current host machine type.
    CHAR const* getIntTypeFormat(bool hex) const
    {
        if (is32bit()) { return hex ? "0x%x" : "%d"; }
        if (is64bit()) { return hex ? "0x%llx" : "%lld"; }
        UNREACHABLE();
        return nullptr;
    }

    bool is32bit() const { return sizeof(UINT) == sizeof(UINT32); }
    bool is64bit() const { return sizeof(UINT) == sizeof(UINT64); }
public:
    IntSetMap()
    {
        m_pool = smpoolCreate(sizeof(V2NMap) * 4, MEM_CONST_SIZE);
        #ifdef _DEBUG_
        m_num_node = 0;
        #endif
        m_rbtn_pool = smpoolCreate(
            sizeof(RBTNode<IntType, IntVal2Node<IntType, MappedObj>*>) * 4,
            MEM_CONST_SIZE);
        m_root_val2node = new V2NMap(m_rbtn_pool);
    }
    virtual ~IntSetMap()
    {
        smpoolDelete(m_rbtn_pool);
        ASSERT0(m_root_val2node);
        delete m_root_val2node;
        m_root_val2node = nullptr;
        smpoolDelete(m_pool);
    }

    //The function maps an integer set to a given object 'mapped'.
    //iset: an integer set.
    //mapped: an object that corresponding to 'iset'.
    void set(Vector<IntType> const& iset, MappedObj mapped)
    {
        UINT num = iset.get_elem_count();
        if (num == 0) { return; }
        VecIdx i = 0;
        IntType ival = iset.get(i);
        V2NMap * mn = m_root_val2node->next.get(ival);
        if (mn == nullptr) {
            mn = allocV2NMap();
            m_root_val2node->next.set(ival, mn);
        }
        for (i++; i < (VecIdx)num; i++) {
            IntType ival = iset.get(i);
            V2NMap * nextmn = mn->next.get(ival);
            if (nextmn == nullptr) {
                nextmn = allocV2NMap();
                mn->next.set(ival, nextmn);
            }
            mn = nextmn;
        }
        ASSERT0(mn);
        ASSERT0(mn->mapped == mapped || mn->mapped == MappedObj(0));
        mn->mapped = mapped;
    }

    //The function maps an integer set to a given object 'mapped'.
    //iset: an integer set.
    //mapped: an object that corresponding to 'set'.
    void set(List<IntType> const& iset, MappedObj mapped)
    {
        UINT num = iset.get_elem_count();
        if (num == 0) { return; }
        VecIdx i = 0;
        typename List<IntType>::Iter it;
        IntType ival = iset.get_head(&it);
        V2NMap * mn = m_root_val2node->next.get(ival);
        if (mn == nullptr) {
            mn = allocV2NMap();
            m_root_val2node->next.set(ival, mn);
        }
        for (i++; i < (VecIdx)num; i++) {
            IntType ival = iset.get_next(&it);
            V2NMap * nextmn = mn->next.get(ival);
            if (nextmn == nullptr) {
                nextmn = allocV2NMap();
                mn->next.set(ival, nextmn);
            }
            mn = nextmn;
        }
        ASSERT0(mn);
        ASSERT0(mn->mapped == mapped || mn->mapped == MappedObj(0));
        mn->mapped = mapped;
    }

    //Count memory usage for current object.
    size_t count_mem() const
    {
        size_t count = smpoolGetPoolSize(m_pool);
        count += smpoolGetPoolSize(m_rbtn_pool);
        count += get_root()->count_mem();
        return count;
    }

    //Dump all table as tree style.
    void dump(FILE * h, UINT indent) const
    {
        if (h == nullptr) { return; }
        log(h, indent, "\n==---- DUMP IntSetMap ----==");
        #ifdef _DEBUG_
        log(h, indent, "\n---- NumOfNode:%d ----",
            //The first node is m_root_val2node
            m_num_node + 1);
        #endif
        dump_helper(h, get_root(), indent);
        log(h, indent, "\n");
        fflush(h);
    }

    //Get the root Node in the table.
    V2NMap const* get_root() const  { return m_root_val2node; }

    //Return true if the function find the a mapped object that corresponding
    //to given integer set.
    //set: an integer set.
    //mapped: record the mapped object if found.
    bool find(Vector<IntType> const& set, OUT MappedObj & mapped) const
    {
        V2NMap const* mn = get_root();
        ASSERT0(mn);
        UINT num = set.get_elem_count();
        for (VecIdx i = 0; i < (VecIdx)num; i++) {
            IntType ival = set.get(i);
            if (mn == nullptr) { return false; }
            V2NMap const* nextmn = mn->next.get(ival);
            if (nextmn == nullptr) { return false; }
            mn = nextmn;
        }
        mapped = mn->mapped;
        return true;
    }

    //Return true if the function find the a mapped object that corresponding
    //to given integer set.
    //set: an integer set.
    //mapped: record the mapped object if found.
    bool find(List<IntType> const& set, OUT MappedObj & mapped) const
    {
        V2NMap const* mn = get_root();
        ASSERT0(mn);
        UINT num = set.get_elem_count();
        typename List<IntType>::Iter it;
        set.get_head(&it);
        for (VecIdx i = 0; i < (VecIdx)num; i++) {
            IntType ival = it->val();
            if (mn == nullptr) { return false; }
            V2NMap const* nextmn = mn->next.get(ival);
            if (nextmn == nullptr) { return false; }
            mn = nextmn;
            set.get_next(&it);
        }
        mapped = mn->mapped;
        return true;
    }
};


//The class represents an example to map an integer set to a string.
template <typename IntType>
class IntSet2StrMap : public IntSetMap<IntType, CHAR const*> {
public:
    //The target dependent code to dump the content of user defined
    //MappedObject.
    virtual void dumpMappedObj(
        FILE * h, UINT indent, CHAR const* const& mapped) const
    {
        if (mapped == nullptr) { return; }
        CHAR const* hexintfmt =
            IntSetMap<IntType, CHAR const*>::getIntTypeFormat(true);
        StrBuf fmt(16);
        fmt.strcat(": mapped_addr:0x%s,mapped_content:%%s", hexintfmt);
        log(h, indent, fmt.buf, mapped, mapped);
    }
};

} //namespace xcom

#endif
