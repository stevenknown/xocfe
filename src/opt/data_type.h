/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

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

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

namespace xoc {

class IR;
class Type;
class TypeMgr;
class VectorType;
class StreamType;
class TensorType;
class RegionMgr;

#define UNDEF_TYID 0

#define IS_INT(t) ((t) >= D_I8 && (t) <= D_U128)
#define IS_SINT(t) ((t) >= D_I8 && (t) <= D_I128)
#define IS_UINT(t) ((t) >= D_U8 && (t) <= D_U128)
#define IS_FP(t) ((t) >= D_F8_E4M3 && (t) <= D_F128)
#define IS_BOOL(t) ((t) == D_B)
#define IS_MC(t) ((t) == D_MC)
#define IS_VEC(t) ((t) == D_VEC)
#define IS_STREAM(t) ((t) == D_STREAM)
#define IS_PTR(t) ((t) == D_PTR)
#define IS_SIMPLEX(t) (IS_INT(t) || IS_FP(t) || IS_BOOL(t) || \
                      t == D_STR || t == D_ANY)

#define SWITCH_CASE_FP_DTYPE \
    case D_F8_E4M3: \
    case D_F8_E5M2: \
    case D_BF16: \
    case D_F16: \
    case D_F32: \
    case D_F64: \
    case D_F80: \
    case D_F128

#define SWITCH_CASE_SINT_DTYPE \
    case D_I8: \
    case D_I16: \
    case D_I32: \
    case D_I64: \
    case D_I128

#define SWITCH_CASE_UINT_DTYPE \
    case D_U8: \
    case D_U16: \
    case D_U32: \
    case D_U64: \
    case D_U128

#define SWITCH_CASE_INT_DTYPE \
    SWITCH_CASE_SINT_DTYPE: \
    SWITCH_CASE_UINT_DTYPE

#define SWITCH_CASE_SIMPLEX_DTYPE \
    SWITCH_CASE_INT_DTYPE: \
    SWITCH_CASE_FP_DTYPE: \
    case D_B: \
    case D_STR: \
    case D_ANY

#define SWITCH_CASE_SMALL_INT_DTYPE \
    case D_B: \
    case D_I8: \
    case D_U8: \
    case D_I16: \
    case D_U16: \
    case D_I32: \
    case D_U32

#define SWITCH_CASE_LONG_INT_DTYPE \
    case D_I64: \
    case D_U64

//Data Type, represented with bit length.
//
//Is unsigned type indispensible?
//
//Java, for example, gets rid of signed/unsigned mixup issues by eliminating
//unsigned types. This goes down a little hard for type convertion, although
//some argue that Java has demonstrated that unsigned types really aren't
//necessary. But unsigned types are useful for implementing memory operation
//for that span more than half of the memory address space, and useful for
//primitives implementing multiprecision arithmetic, etc.
typedef enum _DATA_TYPE {
    D_UNDEF = 0,

    //Note the type between D_B and D_U128 must be integer.
    D_B, //Boolean
    D_I8, //Signed integer 8 bit
    D_I16,
    D_I32,
    D_I64,
    D_I128,
    D_U8, //Unsigned integer 8 bit
    D_U16,
    D_U32,
    D_U64,
    D_U128,

    //Float type.
    D_F8_E4M3, //Float point 8 bit with 1 sign bit, 4 exponent
               //bits, and 3 mantissa bits
    D_F8_E5M2, //Float point 8 bit with 1 sign bit, 5 exponent
               //bits, and 2 mantissa bits
    D_BF16, //BFloat point 16 bit
    D_F16, //Float point 16 bit
    D_F32, //Float point 32 bit
    D_F64, //Float point 64 bit
    D_F80, //Float point 80 bit
    D_F128, //Float point 128 bit
    //NOTE ALL ABOVE TYPES ARE SCALAR.

    D_MC, //MemoryChunk, used to represent structure/union/block type.
    D_STR, //String
    D_PTR, //Pointer
    D_VEC, //Vector
    D_ANY, //Any
    D_TENSOR, //Tensor
    D_STREAM, //Stream
    D_LAST,
} DATA_TYPE;

class TypeDesc {
public:
    DATA_TYPE dtype;
    CHAR const* name;
    UINT bitsize;
};
#define TYDES_dtype(d) ((d)->dtype)
#define TYDES_name(d) ((d)->name)
#define TYDES_bitsize(d) ((d)->bitsize)

#ifdef _DEBUG_
Type const* checkType(Type const* ty, DATA_TYPE dt);
#define CK_TY(ty, dt) (checkType(ty, dt))
#else
//RELEASE MODE
#define CK_TY(ty, dt) (ty)
#endif

#define DTNAME(type) (TYDES_name(&g_type_desc[type]))

//Define target bit size of WORD length.
#define WORD_BITSIZE GENERAL_REGISTER_SIZE * BIT_PER_BYTE

//Data Type Descriptor.
#define TY_dtype(d) ((d)->data_type)

//Indicate the pointer base size.
#define TY_ptr_base_size(d) (((PointerType*)CK_TY(d, D_PTR))->pointer_base_size)

//Indicate the size of memory chunk.
#define TY_mc_size(d) (((MCType*)CK_TY(d, D_MC))->mc_size)

//Indicate the total byte size of whole vector.
#define TY_vec_size(d) (((VectorType*)CK_TY(d, D_VEC))->total_vector_size)

//Indicate the vector element data type.
#define TY_vec_ety(d) (((VectorType*)CK_TY(d, D_VEC))->vector_elem_type)

//Indicate the tensor element data type.
#define TY_tensor_ety(d) (((TensorType*)CK_TY(d, D_TENSOR))->tensor_elem_type)

//Indicate the stream element data type.
#define TY_stream_ety(d) (((StreamType*)CK_TY(d, D_STREAM))->stream_elem_type)

//Data Type Description.
class Type {
    COPY_CONSTRUCTOR(Type);
public:
    DATA_TYPE data_type;
public:
    Type() { data_type = D_UNDEF; }

    void copy(Type const& src) { data_type = src.data_type; }

    inline void dump(TypeMgr const* tm) const;

    //Return data type.
    DATA_TYPE getDType() const { return TY_dtype(this); }

    //Return the number of elements in the vector.
    inline UINT getVectorElemNum(TypeMgr const* tm) const;

    //Return element data-type of element.
    inline DATA_TYPE getVectorElemDType() const;

    //Return element data-type of element.
    inline DATA_TYPE getTensorElemDType() const;

    //Return element type of element.
    inline Type const* getVectorElemType(TypeMgr const* tm) const;

    //Return element type of element.
    inline Type const* getTensorElemType(TypeMgr const* tm) const;

    //Return byte size of element.
    inline UINT getVectorElemSize(TypeMgr const* tm) const;

    //Return byte size of element.
    inline UINT getTensorElemSize(TypeMgr const* tm) const;

    //Return element type of element.
    inline DATA_TYPE getStreamElemType() const;

    //Return true if data type is simplex type.
    bool is_simplex() const { return IS_SIMPLEX(TY_dtype(this)); }

    //Return true if data type is vector.
    bool is_vector() const { return TY_dtype(this) == D_VEC; }

    //Return true if data type is vector and its element type is float.
    bool is_vector_with_fp_elem_type() const
    { return is_vector() && is_fp(getVectorElemDType()); }

    //Return true if data type is vector and its element type is bool.
    bool is_vector_with_bool_elem_type() const
    { return is_vector() && getVectorElemDType() == D_B; }

    //Return true if data type is stream.
    bool is_stream() const { return TY_dtype(this) == D_STREAM; }

    //Return true if data type is pointer.
    bool is_pointer() const { return TY_dtype(this) == D_PTR; }

    //Return true if data type is string.
    bool is_string() const { return TY_dtype(this) == D_STR; }

    //Return true if data type is memory chunk.
    bool is_mc() const { return TY_dtype(this) == D_MC; }

    //Return true if data type is any.
    bool is_any() const { return TY_dtype(this) == D_ANY; }

    //Return true if data type is tensor.
    bool is_tensor() const { return TY_dtype(this) == D_TENSOR; }

    //Return true if data type is tensor and its element type is float.
    bool is_tensor_with_fp_elem_type() const
    { return is_tensor() && is_fp(getTensorElemDType()); }

    //Return true if data type is boolean.
    bool is_bool() const { return TY_dtype(this) == D_B; }
    bool is_i8() const { return TY_dtype(this) == D_I8; }
    bool is_i16() const { return TY_dtype(this) == D_I16; }
    bool is_i32() const { return TY_dtype(this) == D_I32; }
    bool is_i64() const { return TY_dtype(this) == D_I64; }
    bool is_i128() const { return TY_dtype(this) == D_I128; }
    bool is_u8() const { return TY_dtype(this) == D_U8; }
    bool is_u16() const { return TY_dtype(this) == D_U16; }
    bool is_u32() const { return TY_dtype(this) == D_U32; }
    bool is_u64() const { return TY_dtype(this) == D_U64; }
    bool is_u128() const { return TY_dtype(this) == D_U128; }
    bool is_f8_e4m3() const { return TY_dtype(this) == D_F8_E4M3; }
    bool is_f8_e5m2() const { return TY_dtype(this) == D_F8_E5M2; }
    bool is_bf16() const { return TY_dtype(this) == D_BF16; }
    bool is_f16() const { return TY_dtype(this) == D_F16; }
    bool is_f32() const { return TY_dtype(this) == D_F32; }
    bool is_f64() const { return TY_dtype(this) == D_F64; }
    bool is_f128() const { return TY_dtype(this) == D_F128; }

    //Return true if data type is primitive.
    bool is_scalar() const
    { return TY_dtype(this) >= D_B && TY_dtype(this) <= D_F128; }

    //Return true if current type is signed.
    inline bool is_signed() const
    {
        if ((TY_dtype(this) >= D_I8 && TY_dtype(this) <= D_I128) ||
            (TY_dtype(this) >= D_F8_E4M3 && TY_dtype(this) <= D_F128)) {
            return true;
        }
        return false;
    }

    //Return true if current type is unsigned.
    inline bool is_unsigned() const
    {
        if ((TY_dtype(this) >= D_U8 && TY_dtype(this) <= D_U128) ||
            TY_dtype(this) == D_STR ||
            TY_dtype(this) == D_PTR ||
            TY_dtype(this) == D_VEC ||
            TY_dtype(this) == D_STREAM) {
            return true;
        }
        return false;
    }

    //Return true if the type can be regarded as signed.
    bool isSigned() const
    {
        if (is_vector()) { return is_signed(getVectorElemDType()); }
        return is_signed();
    }

    //Return true if the type can be regarded as unsigned.
    bool isUnsigned() const
    {
        if (is_vector()) { return is_unsigned(getVectorElemDType()); }
        return is_unsigned();
    }

    //Return true if data type is signed integer.
    bool is_sint() const
    { return TY_dtype(this) >= D_I8 && TY_dtype(this) <= D_I128; }

    //Return true if data type is unsigned integer.
    bool is_uint() const
    { return TY_dtype(this) >= D_U8 && TY_dtype(this) <= D_U128; }

    //Return true if data type is integer.
    bool is_int() const
    { return TY_dtype(this) >= D_B && TY_dtype(this) <= D_U128; }

    //Return true if data type is subword integer.
    bool is_subword_int() const
    {
        return (TY_dtype(this) >= D_B && TY_dtype(this) <= D_I16) ||
               (TY_dtype(this) >= D_U8 && TY_dtype(this) <= D_U16);
    }

    //Return true if data type is float.
    bool is_fp() const { return is_fp(TY_dtype(this)); }

    //Return true if data type is float.
    static bool is_fp(DATA_TYPE dtype)
    { return dtype >= D_F8_E4M3 && dtype <= D_F128; }

    //Return true if data type is signed integer.
    static bool is_signed(DATA_TYPE dtype)
    { return dtype >= D_I8 && dtype <= D_I128; }

    //Return true if data type is unsigned integer.
    static bool is_unsigned(DATA_TYPE dtype)
    { return dtype >= D_U8 && dtype <= D_U128; }

    //Return true if data type can be regarded as integer.
    bool isInt() const { return is_int() || is_bool() || is_pointer(); }

    //Return true if the type can be used to represent the
    //pointer's addend.
    //e.g:The pointer arith, int * p; p = p + (type)value. value is addend.
    bool is_ptr_addend() const
    { return !is_fp() && !is_mc() && !is_bool() && !is_pointer(); }

    //Return true if the type can be regarded as pointer.
    bool isPointer() const { return is_pointer() || is_any(); }

    //Return true if the type can be regarded as float.
    bool isFP() const
    {
        return is_fp() || is_vector_with_fp_elem_type() ||
               is_tensor_with_fp_elem_type();
    }

    bool verify(TypeMgr const* tm) const;
};


//The class represents Pointer Data Type.
class PointerType : public Type {
    COPY_CONSTRUCTOR(PointerType);
public:
    //e.g  int * p, base size is 4,
    //long long * p, base size is 8
    //char * p, base size is 1.
    UINT pointer_base_size;

public:
    PointerType() { TY_dtype(this) = D_PTR; pointer_base_size = 0; }

    void copy(PointerType const& src)
    {
        Type::copy(src);
        pointer_base_size = src.pointer_base_size;
    }
};


//The class represents Memory Chunk Data Type.
class MCType : public Type {
    COPY_CONSTRUCTOR(MCType);
public:
    //Record the BYTE size if 'rtype' is D_MC.
    //NOTE: If ir is pointer, its 'rtype' should NOT be D_MC.
    UINT mc_size;
public:
    MCType() { TY_dtype(this) = D_MC; mc_size = 0; }

    void copy(MCType const& src)
    {
        Type::copy(src);
        mc_size = src.mc_size;
    }

    //Return the byte size of the memory chunk.
    UINT getByteSize() const { return TY_mc_size(this); }
};


//The class represents Vector Data Type.
class VectorType : public Type {
    COPY_CONSTRUCTOR(VectorType);
public:
    //Record the BYTE size of total vector.
    UINT total_vector_size;

    //Record the vector element type.
    //Note the element can only be simplex type.
    DATA_TYPE vector_elem_type;
public:
    VectorType()
    {
        TY_dtype(this) = D_VEC;
        total_vector_size = 0;
        vector_elem_type = D_UNDEF;
    }

    void copy(VectorType const& src)
    {
        Type::copy(src);
        total_vector_size = src.total_vector_size;
        vector_elem_type = src.vector_elem_type;
    }

    //Return the byte size of the vector.
    UINT getByteSize() const { return TY_vec_size(this); }
};


//The class represents variable-length Stream Data Type.
class StreamType : public Type {
    COPY_CONSTRUCTOR(StreamType);
public:
    //Record the stream element type.
    //Note the element can only be simplex type.
    DATA_TYPE stream_elem_type;
public:
    StreamType()
    {
        TY_dtype(this) = D_STREAM;
        stream_elem_type = D_UNDEF;
    }

    void copy(StreamType const& src)
    {
        Type::copy(src);
        stream_elem_type = src.stream_elem_type;
    }
};


//The class represents Tensor Data Type.
class TensorType : public Type {
    COPY_CONSTRUCTOR(TensorType);
public:
    friend class TypeMgr;

    //Note the default grow size should be 1 because, SimpleVector does not
    //have 'lastidx' indicator, thus it just regards the maximum size of
    //vector as the number of elements.
    static UINT const g_default_grow_size = 1;
    static UINT const g_default_max_tensor_dim = 64;

    //Record the degree of each dimension.
    //e.g: <3x2x7x1>, the degree of dimension 0 is 3,
    //degree of dimension 1 is 2, degree of dimension 2 is 7, etc.
    xcom::SimpleVector<UINT, g_default_grow_size, g_default_max_tensor_dim>
        degree_of_dimension;

    //Record data type of element in tensor.
    DATA_TYPE tensor_elem_type;
public:
    TensorType()
    {
        TY_dtype(this) = D_TENSOR;
        tensor_elem_type = D_UNDEF;
    }

    void copy(TensorType const& src, TypeMgr * mgr);

    void destroy() { degree_of_dimension.destroy(); }

    //Get data type of element in tensor.
    DATA_TYPE getElemDataType() const { return tensor_elem_type; }

    //Get type of element in tensor.
    inline Type const* getElemType(TypeMgr const* tm) const;

    //Return the degree of given dimension in tensor.
    UINT getDegreeOfDim(UINT dim) const
    { return degree_of_dimension.get(dim); }

    //Return the number of dimensions of tensor.
    UINT getDim() const { return degree_of_dimension.get_capacity(); }

    //Return the byte size of total tensor.
    UINT getByteSize(TypeMgr const* mgr) const;

    void init() { degree_of_dimension.init(); }

    //Return true if data type is signed integer.
    bool is_sint() const { return IS_SINT(tensor_elem_type); }

    //Return true if data type is unsgined integer.
    bool is_uint() const { return IS_UINT(tensor_elem_type); }

    //Return true if data type is integer.
    //The function treats BOOL as integer.
    bool is_int() const
    { return tensor_elem_type >= D_B && tensor_elem_type <= D_U128; }

    //Return true if data type is float.
    bool is_fp() const { return IS_FP(tensor_elem_type); }

    //Return true if curent tensor type's dimension and the dergree of each
    //dimensions are equal to 'src'.
    //NOTE the function does NOT check the element type.
    //e.g: If current tensor is <i32x3x3x6>, src is <f64x3x3x6>, the function
    //return true. If current tensor is <f64x3x3>, src is <f64x3x4>, the
    //function return false.
    bool is_homo(TensorType const& src) const;

    //Set degree to given dimension.
    void setDegreeOfDim(UINT dim, UINT degree, TypeMgr * mgr);
};


//Container of Type.
#define TC_type(c) ((c)->ty)
#define TC_typeid(c) ((c)->tyid)
class TypeContainer {
public:
    Type * ty;
    UINT tyid;
};


class CompareTypeMC {
public:
    bool is_less(Type const* t1, Type const* t2) const
    {
        ASSERT0(t1 && t2);
        return TY_mc_size(t1) < TY_mc_size(t2);
    }
    bool is_equ(Type const* t1, Type const* t2) const
    {
        ASSERT0(t1 && t2);
        return TY_mc_size(t1) == TY_mc_size(t2);
    }
    Type const* createKey(Type const* t) { return t; }
};


class MCTab : public TMap<Type const*, TypeContainer const*, CompareTypeMC> {
public:
};


class CompareTypePointer {
public:
    bool is_less(Type const* t1, Type const* t2) const
    { return TY_ptr_base_size(t1) < TY_ptr_base_size(t2); }

    bool is_equ(Type const* t1, Type const* t2) const
    { return TY_ptr_base_size(t1) == TY_ptr_base_size(t2); }

    Type const* createKey(Type const* t) { return t; }
};


class PointerTab : public
    TMap<Type const*, TypeContainer const*, CompareTypePointer> {
public:
};


//Comparison of data type of element in vector.
class CompareTypeVectoElemType {
public:
    bool is_less(Type const* t1, Type const* t2) const
    { return TY_vec_ety(t1) < TY_vec_ety(t2); }

    bool is_equ(Type const* t1, Type const* t2) const
    { return TY_vec_ety(t1) == TY_vec_ety(t2); }

    Type const* createKey(Type const* t) { return t; }
};


//Comparison of data type of element in stream.
class CompareTypeStreamElemType {
public:
    bool is_less(Type const* t1, Type const* t2) const
    { return TY_stream_ety(t1) < TY_stream_ety(t2); }

    bool is_equ(Type const* t1, Type const* t2) const
    { return TY_stream_ety(t1) == TY_stream_ety(t2); }

    Type const* createKey(Type const* t) { return t; }
};


//Comparison of data type of element in tensor.
class CompareTypeTensorElemType {
public:
    bool is_less(Type const* t1, Type const* t2) const
    {
        ASSERT0(t1 && t2);
        UINT dim1 = ((TensorType const*)t1)->getDim();
        UINT dim2 = ((TensorType const*)t2)->getDim();
        if (dim1 < dim2) {
            return true;
        }
        if (dim1 > dim2) {
            return false;
        }
        for (UINT i = 0; i < dim1; i++) {
            if (((TensorType const*)t1)->getDegreeOfDim(i) <
                ((TensorType const*)t2)->getDegreeOfDim(i)) {
                return true;
            }
        }
        return false;
    }

    bool is_equ(Type const* t1, Type const* t2) const
    {
        ASSERT0(t1 && t2);
        UINT dim1 = ((TensorType const*)t1)->getDim();
        if (dim1 != ((TensorType const*)t2)->getDim()) {
            return false;
        }
        for (UINT i = 0; i < dim1; i++) {
            if (((TensorType const*)t1)->getDegreeOfDim(i) !=
                ((TensorType const*)t2)->getDegreeOfDim(i)) {
                return false;
            }
        }
        return true;
    }

    Type const* createKey(Type const* t) { return t; }
};


//Comparison of data type of element in tensor.
class CompareTypeTensor {
public:
    bool is_less(Type const* t1, Type const* t2) const
    { return TY_tensor_ety(t1) < TY_tensor_ety(t2); }

    bool is_equ(Type const* t1, Type const* t2) const
    { return TY_tensor_ety(t1) == TY_tensor_ety(t2); }

    Type const* createKey(Type const* t) { return t; }
};


class VectorElemTypeTab : public
    TMap<Type const*, TypeContainer const*, CompareTypeVectoElemType> {
};

class StreamElemTypeTab : public
    TMap<Type const*, TypeContainer const*, CompareTypeStreamElemType> {
};

class TensorElemTypeTab : public
    TMap<Type const*, TypeContainer const*, CompareTypeTensorElemType> {
};

typedef xcom::TMapIter<Type const*, VectorElemTypeTab*> VectorElemTypeTabIter;
typedef xcom::TMapIter<Type const*, TensorElemTypeTab*> TensorElemTypeTabIter;


class CompareTypeVector {
public:
    bool is_less(Type const* t1, Type const* t2) const
    { return TY_vec_size(t1) < TY_vec_size(t2); }

    bool is_equ(Type const* t1, Type const* t2) const
    { return TY_vec_size(t1) == TY_vec_size(t2); }

    Type const* createKey(Type const* t) { return t; }
};


//Type Table that record all registered vector type.
class VectorTab : public
  xcom::TMap<Type const*, VectorElemTypeTab*, CompareTypeVector> {
public:
};

//Type Table that record all registered tensor type.
class TensorTab : public
  xcom::TMap<Type const*, TensorElemTypeTab*, CompareTypeTensor> {
public:
};


extern TypeDesc const g_type_desc[];

//
//START TypeMgr
//
class TypeMgr {
    COPY_CONSTRUCTOR(TypeMgr);
    friend class TensorType;
protected:
    RegionMgr * m_rm;
    xcom::Vector<Type*> m_type_tab;
    SMemPool * m_pool;
    PointerTab m_pointer_type_tab;
    MCTab m_memorychunk_type_tab;
    VectorTab m_vector_type_tab;
    StreamElemTypeTab m_stream_type_tab;
    TensorTab m_tensor_type_tab;
    TypeContainer * m_simplex_type[D_LAST];
    UINT m_type_count;
    Type const* m_any;
    Type const* m_b;
    Type const* m_i8;
    Type const* m_i16;
    Type const* m_i32;
    Type const* m_i64;
    Type const* m_i128;
    Type const* m_u8;
    Type const* m_u16;
    Type const* m_u32;
    Type const* m_u64;
    Type const* m_u128;
    Type const* m_f8e4m3;
    Type const* m_f8e5m2;
    Type const* m_bf16;
    Type const* m_f16;
    Type const* m_f32;
    Type const* m_f64;
    Type const* m_f80;
    Type const* m_f128;
    Type const* m_str;
protected:
    SMemPool * get_pool() const { return m_pool; }

    Type * newType() { return (Type*)xmalloc(sizeof(Type)); }

    VectorType * newVectorType()
    { return (VectorType*)xmalloc(sizeof(VectorType)); }

    StreamType * newStreamType()
    { return (StreamType*)xmalloc(sizeof(StreamType)); }

    TensorType * newTensorType()
    {
        TensorType * t = (TensorType*)xmalloc(sizeof(TensorType));
        t->init();
        return t;
    }

    MCType * newMCType() { return (MCType*)xmalloc(sizeof(MCType)); }

    PointerType * newPointerType()
    { return (PointerType*)xmalloc(sizeof(PointerType)); }

    //Alloc TypeContainer.
    TypeContainer * newTC()
    { return (TypeContainer*)xmalloc(sizeof(TypeContainer)); }

    void * xmalloc(size_t size)
    {
        void * p = smpoolMalloc(size, m_pool);
        ASSERT0(p);
        ::memset((void*)p, 0, size);
        return p;
    }
public:
    TypeMgr(RegionMgr * rm);
    virtual ~TypeMgr();

    template<class StrBufType>
    CHAR const* dump_type(Type const* ty, OUT StrBufType & buf) const;
    void dump_type(Type const* ty) const;
    void dump_type(UINT tyid) const;
    void dump_type_tab() const;

    DATA_TYPE hoistBSdtype(UINT bit_size, bool is_signed) const;
    DATA_TYPE hoistDtype(UINT bit_size, OUT UINT * hoisted_data_size);
    DATA_TYPE hoistDtype(DATA_TYPE stype) const;

    Type const* hoistDTypeForBinOp(IR const* opnd0, IR const* opnd1);

    RegionMgr * getRegionMgr() const { return m_rm; }

    //Return DATA_TYPE which 'bitsize' corresponding to.
    //is_bf: set to true if current type is bfloat.
    DATA_TYPE getFPDType(UINT bitsize, bool is_bf) const
    {
        switch (bitsize) {
        case 16: return is_bf ? D_BF16 : D_F16;
        case 32: return D_F32;
        case 64: return D_F64;
        case 128: return D_F128;
        default:;
        }
        return D_UNDEF;
    }

    //Return Data-Type according to given bit size and sign.
    //If bitsize is not equal to 1, 8, 16, 32, 64, 128, this
    //function will return D_MC.
    DATA_TYPE getIntDType(UINT bitsize, bool is_signed) const
    {
        switch (bitsize) {
        case 1: return D_B;
        case 8: return is_signed ? D_I8 : D_U8;
        case 16: return is_signed ? D_I16 : D_U16;
        case 32: return is_signed ? D_I32 : D_U32;
        case 64: return is_signed ? D_I64 : D_U64;
        case 128: return is_signed ? D_I128 : D_U128;
        default: break;
        }
        return D_MC;
    }

    //Return DATA_TYPE which 'bitsize' corresponding to.
    //is_bf: set to true if current type is bfloat.
    Type const* getFPType(UINT bitsize, bool is_bf) const
    { return getSimplexTypeEx(getFPDType(bitsize, is_bf)); }

    //Return Integer Type according to given bit size and sign.
    //e.g: given bitsize is 32, is_signed is true, return D_I32 type.
    Type const* getIntType(UINT bitsize, bool is_signed) const
    { return getSimplexTypeEx(getIntDType(bitsize, is_signed)); }

    //Return Signed Integer Type of host machine.
    Type const* getHostIntType() const
    { return getIntType(sizeof(HOST_INT) * HOST_BIT_PER_BYTE, true); }

    //Return Unsigned Integer Type of host machine.
    Type const* getHostUIntType() const
    { return getIntType(sizeof(HOST_UINT) * HOST_BIT_PER_BYTE, false); }

    //Return Float-Point Type of host machine.
    //is_bf: set to true if current type is bfloat. By default, host
    //       float-point type is usually be float or double.
    virtual Type const* getHostFPType(bool is_bf = false) const
    { return getFPType(sizeof(HOST_FP) * HOST_BIT_PER_BYTE, is_bf); }

    //Return Unsigned Integer Data-Type according to given byte size.
    DATA_TYPE getUIntDType(UINT bytesize) const
    { return getIntDType(bytesize * BIT_PER_BYTE, false); }

    //Return Signed Integer Data-Type according to given byte size.
    DATA_TYPE getSIntDType(UINT bytesize) const
    { return getIntDType(bytesize * BIT_PER_BYTE, true); }

    //Return Signed Integer Type that has same byte size with given 'ty'.
    //e.g: given ty is F64, return I64.
    Type const* getSIntTypeWithSameSize(Type const* ty) const
    { return getIntType(getBitSize(ty), true); }

    //Return Unsigned Integer or MemoryChunk Type according to given byte size.
    Type const* getUIntType(UINT bytesize)
    {
        DATA_TYPE dt = getIntDType(bytesize * BITS_PER_BYTE, false);
        return dt == D_MC ? getMCType(bytesize) : getSimplexTypeEx(dt);
    }

    //Return Unsigned Integer Type that has same byte size with given 'ty'.
    //e.g: given ty is F64, return U64.
    Type const* getUIntTypeWithSameSize(Type const* ty) const
    { return getIntType(getBitSize(ty), false); }

    //Return Data-Type according to given bit size and sign.
    //Note the bit size will be aligned to power of 2.
    DATA_TYPE getAlignedDType(UINT bit_size, bool is_signed) const
    { return hoistBSdtype(bit_size, is_signed); }

    //Return bits size of 'dtype' refers to.
    UINT getDTypeBitSize(DATA_TYPE dtype) const
    {
        ASSERTN(dtype != D_MC && dtype != D_TENSOR, ("complex type"));
        return TYDES_bitsize(&g_type_desc[dtype]);
    }

    //Return bits size of 'dtype' refers to.
    static CHAR const* getDTypeName(DATA_TYPE dtype)
    {
        ASSERT0(dtype < D_LAST);
        return TYDES_name(&g_type_desc[dtype]);
    }

    //Return byte size of a pointer.
    //e.g: 32bit processor return 4, 64bit processor return 8.
    UINT getPointerByteSize() const { return BYTE_PER_POINTER; }

    //Return bit size of a pointer.
    //e.g: 32bit processor return 32, 64bit processor return 64.
    UINT getPointerBitSize() const
    { return getPointerByteSize() * BIT_PER_BYTE; }

    //Return DATA-Type that has identical byte-size to pointer.
    //e.g: 32bit processor return U4, 64bit processor return U8.
    DATA_TYPE getPointerSizeDtype() const
    { return getIntDType(BYTE_PER_POINTER * BIT_PER_BYTE, false); }

    //Return Type that has identical byte-size to pointer.
    //e.g: 32bit processor return U4, 64bit processor return U8.
    Type const* getPointerSizeType() const
    { return getSimplexTypeEx(getPointerSizeDtype()); }

    //Return the byte size of pointer's base.
    UINT getPointerBaseByteSize(Type const* ty) const
    {
        ASSERT0(ty->is_pointer());
        return TY_ptr_base_size(ty);
    }

    //Return bytes size of 'dtype' refer to.
    UINT getDTypeByteSize(DATA_TYPE dtype) const
    {
        ASSERT0(dtype != D_UNDEF);
        UINT bitsize = getDTypeBitSize(dtype);
        return bitsize < BIT_PER_BYTE ?
            (UINT)1 : (UINT)xceiling((INT)bitsize, BIT_PER_BYTE);
    }

    //Retrieve Type via 'type-index'.
    Type const* getType(UINT tyid) const
    {
        ASSERT0(tyid != 0);
        ASSERT0(m_type_tab.get(tyid));
        return m_type_tab.get(tyid);
    }

    Type const* getBool() const { return m_b; }
    Type const* getI8() const { return m_i8; }
    Type const* getI16() const { return m_i16; }
    Type const* getI32() const { return m_i32; }
    Type const* getI64() const { return m_i64; }
    Type const* getI128() const { return m_i128; }
    Type const* getU8() const { return m_u8; }
    Type const* getU16() const { return m_u16; }
    Type const* getU32() const { return m_u32; }
    Type const* getU64() const { return m_u64; }
    Type const* getU128() const { return m_u128; }
    Type const* getBF16() const { return m_bf16; }
    Type const* getF16() const { return m_f16; }
    Type const* getF32() const { return m_f32; }
    Type const* getF64() const { return m_f64; }
    Type const* getF80() const { return m_f80; }
    Type const* getF128() const { return m_f128; }
    Type const* getString() const { return m_str; }
    Type const* getAny() const { return m_any; }

    //Generate and return type according to given DATA_TYPE.
    Type const* getSimplexType(DATA_TYPE dt)
    {
        ASSERT0(IS_SIMPLEX(dt));
        Type d;
        TY_dtype(&d) = dt;
        return registerType(&d);
    }

    //Return existing type according to given DATA_TYPE.
    inline Type const* getSimplexTypeEx(DATA_TYPE dt) const
    {
        switch (dt) {
        case D_B: return m_b;
        case D_I8: return m_i8;
        case D_I16: return m_i16;
        case D_I32: return m_i32;
        case D_I64: return m_i64;
        case D_I128: return m_i128;
        case D_U8: return m_u8;
        case D_U16: return m_u16;
        case D_U32: return m_u32;
        case D_U64: return m_u64;
        case D_U128: return m_u128;
        case D_F8_E4M3: return m_f8e4m3;
        case D_F8_E5M2: return m_f8e5m2;
        case D_BF16: return m_bf16;
        case D_F16: return m_f16;
        case D_F32: return m_f32;
        case D_F64: return m_f64;
        case D_F80: return m_f80;
        case D_F128: return m_f128;
        case D_STR: return m_str;
        case D_ANY: return m_any;
        default: ASSERTN(0, ("not simplex type")); break;
        }
        return 0;
    }

    //Return the type that represents the general purpose register of
    //target machine.
    Type const* getTargMachRegisterType() const
    {
        DATA_TYPE dt = getAlignedDType(WORD_LENGTH_OF_TARGET_MACHINE, false);
        Type const* ty = getSimplexTypeEx(dt);
        return ty;
    }

    //Return the type that represents the maximum byte size of vector register
    //of target machine.
    virtual Type const* getTargMachMaxVectorRegisterType()
    {
        ASSERTN(0, ("Target Dependent Code"));
        return nullptr;
    }

    //Return tensor type, total byte size of tensor =
    //degree_of_dim0 * degree_of_dim1 * ...  * degree_of_dimN * elem_byte_size.
    //e.g: Get tensor with type D_F32<2x3x4x5x1>.
    // Type const* tensor = getTensorType(D_F32, 4, 2, 3, 4, 5, 1);
    // Return type indicates there are 120 elements in tensor,
    // each element is D_F32, the degree of dimension 0 is 2, and degree of
    // dimenson 1 is 3, and so on. Total size of tensor is 480 bytes.
    Type const* getTensorType(DATA_TYPE elem_ty, UINT dim, ...);

    //Return vector type, and vector total size = <vec_elem_num x vec_elem_ty>.
    //e.g: int<16 x D_I32> means there are 16 elems in vector, each elem is
    //D_I32 type, and vector total size is 64 bytes.
    Type const* getVectorType(UINT vec_elem_num, DATA_TYPE vec_elem_ty)
    {
        ASSERT0(vec_elem_num != 0 && vec_elem_ty != D_UNDEF);
        VectorType d;
        TY_dtype(&d) = D_VEC;
        TY_vec_size(&d) = vec_elem_num * getDTypeByteSize(vec_elem_ty);
        TY_vec_ety(&d) = vec_elem_ty;
        return TC_type(registerVector(&d));
    }

    //Return vector type, and vector total size = <vec_elem_num x vec_elem_ty>.
    //e.g: int<16 x D_I32> means there are 16 elems in vector, each elem is
    //D_I32 type, and vector total size is 64 bytes.
    Type const* getVectorTypeViaTotalSize(
        UINT vec_total_size, DATA_TYPE vec_elem_ty)
    {
        UINT etysize = getDTypeByteSize(vec_elem_ty);
        ASSERT0(etysize);
        ASSERT0(vec_total_size >= etysize);
        ASSERT0((vec_total_size % etysize) == 0);
        UINT vec_elem_num = vec_total_size / etysize;
        return getVectorType(vec_elem_num, vec_elem_ty);
    }

    //Return stream type, which element type is 'elem_ty'.
    //e.g: stream<D_I32> means this is a stream type with each element is I32,
    //and the stream has variable-length.
    Type const* getStreamType(DATA_TYPE elem_ty)
    {
        ASSERT0(elem_ty != D_UNDEF);
        StreamType d;
        TY_dtype(&d) = D_STREAM;
        TY_stream_ety(&d) = elem_ty;
        return TC_type(registerStream(&d));
    }

    //Register and return pointer type according to pt_base_size.
    //pt_base_sz: byte size of pointer's base type.
    Type const* getPointerType(UINT pt_base_sz)
    {
        //Pointer base size could be zero.
        //Note if pointer base size is 0, that means the pointer can not
        //do any arthimetic, because pointer arithmetic may use pointer
        //base size as an addend.
        //ASSERT0(pt_base_sz != 0);
        PointerType d;
        TY_dtype(&d) = D_PTR;
        TY_ptr_base_size(&d) = pt_base_sz;
        return TC_type(registerPointer(&d));
    }

    //Return memory chunk tyid according to chunk size and vector element tyid.
    //mc_bytesize: the byte size of memory chunk.
    Type const* getMCType(UINT mc_bytesize)
    {
        MCType d;
        TY_dtype(&d) = D_MC;
        TY_mc_size(&d) = mc_bytesize;
        return TC_type(registerMC(&d));
    }

    //Return byte size according to given Type.
    UINT getByteSize(Type const* ty) const;

    //Return byte size according to given tyid.
    UINT getByteSize(UINT tyid) const { return getByteSize(getType(tyid)); }

    //Return byte size according to given Type.
    UINT getBitSize(Type const* ty) const
    { return getByteSize(ty) * BITS_PER_BYTE; }

    bool is_scalar(UINT tyid) { return tyid >= D_B && tyid <= D_F128; }

    //Return true if tyid is signed.
    bool is_signed(UINT tyid) const { return getType(tyid)->is_signed(); }

    //Return true if tyid is signed integer.
    bool is_sint(UINT tyid) const { return getType(tyid)->is_sint(); }

    //Return true if tyid is unsigned integer.
    bool is_uint(UINT tyid) const { return getType(tyid)->is_uint(); }

    //Return true if tyid is Float point.
    bool is_fp(UINT tyid) const { return getType(tyid)->is_fp(); }

    //Return true if tyid is Boolean.
    bool is_bool(UINT tyid) const { return m_b == getType(tyid); }

    //Return true if tyid is String.
    bool is_str(UINT tyid) const { return m_str == getType(tyid); }

    //Return true if tyid is Memory chunk.
    bool is_mc(UINT tyid) const { return getType(tyid)->is_mc(); }

    //Return true if tyid is Pointer.
    bool is_ptr(UINT tyid) const { return getType(tyid)->is_pointer(); }

    //Return true if tyid is Any.
    bool is_any(UINT tyid) const { return getType(tyid)->is_any(); }

    //Return true if data type is Vector.
    bool is_vec(UINT tyid) const { return getType(tyid)->is_vector(); }

    //Return true if host-float-point value a is equal to b.
    //epsilon: error range.
    static bool isEqual(HOST_FP a, HOST_FP b, HOST_FP epsilon = EPSILON)
    { return ::fabs(a - b) < epsilon; }

    //Register a memory-chunk data type.
    TypeContainer const* registerMC(Type const* ty);

    //Register a vector data type.
    //type: it must be D_VEC type, and the vector-element-type can not D_UNDEF,
    //e.g: vector<I8,I8,I8,I8> type, which mc_size is 32 byte, vec-type is D_I8.
    TypeContainer const* registerVector(Type const* ty);

    //Register a stream data type.
    //ty: it must be D_STREAM type, and the stream-element-type can not D_UNDEF,
    //e.g: stream<I8> type, which stream-element-type is D_I8.
    TypeContainer const* registerStream(Type const* ty);

    //Register a tensor data type.
    //'type': it must be D_TENSOR type, and the
    //tensor-element-type can not D_UNDEF.
    TypeContainer const* registerTensor(Type const* ty);

    //Register a pointer data type.
    TypeContainer const* registerPointer(Type const* ty);

    //Register simplex type container, e.g:INT, UINT, FP, bool.
    TypeContainer const* registerSimplex(Type const* ty);

    //Return data type that registered in m_type_tab.
    Type * registerType(Type const* ty);
};


template<class StrBufType>
CHAR const* TypeMgr::dump_type(Type const* type, OUT StrBufType & buf) const
{
    ASSERT0(type);
    DATA_TYPE dt = TY_dtype(type);
    switch (dt) {
    SWITCH_CASE_INT_DTYPE:
    SWITCH_CASE_FP_DTYPE:
    case D_B:
    case D_STR:
        buf.strcat("%s", DTNAME(dt));
        break;
    case D_MC:
        buf.strcat("%s<%d>", DTNAME(dt), getByteSize(type));
        break;
    case D_PTR:
        buf.strcat("%s<%d>", DTNAME(dt), TY_ptr_base_size(type));
        break;
    case D_VEC: {
        UINT elem_byte_size = getDTypeByteSize(TY_vec_ety(type));
        ASSERT0(elem_byte_size != 0);
        ASSERT0(getByteSize(type) % elem_byte_size == 0);
        UINT elemnum = getByteSize(type) / elem_byte_size;
        buf.strcat("%s<%s*%u>", DTNAME(dt), DTNAME(TY_vec_ety(type)), elemnum);
        break;
    }
    case D_TENSOR: {
        //Check byte size of element.
        ASSERT0(getDTypeByteSize(TY_tensor_ety(type)) != 0);
        ASSERT0(getByteSize(type) % getDTypeByteSize(TY_tensor_ety(type)) == 0);
        buf.strcat("%s:%s<", DTNAME(dt), DTNAME(TY_tensor_ety(type)));
        UINT dim = ((TensorType const*)type)->getDim();
        for (UINT i = 0; i < dim; i++) {
            if (i != 0) {
                buf.strcat("x");
            }
            buf.strcat("%d", ((TensorType const*)type)->getDegreeOfDim(i));
        }
        buf.strcat(">");
        break;
    }
    case D_ANY:
        buf.strcat("%s", DTNAME(dt));
        break;
    default: UNREACHABLE();
    }
    return buf.getBuf();
}
//END TypeMgr


//
//START Type
//
Type const* Type::getVectorElemType(TypeMgr const* tm) const
{
    return tm->getSimplexTypeEx(TY_vec_ety(this));
}


Type const* Type::getTensorElemType(TypeMgr const* tm) const
{
    return tm->getSimplexTypeEx(TY_tensor_ety(this));
}


UINT Type::getVectorElemSize(TypeMgr const* tm) const
{
    return tm->getDTypeByteSize(TY_vec_ety(this));
}


UINT Type::getTensorElemSize(TypeMgr const* tm) const
{
    return tm->getDTypeByteSize(TY_tensor_ety(this));
}


DATA_TYPE Type::getVectorElemDType() const
{
    return TY_vec_ety(this);
}


DATA_TYPE Type::getTensorElemDType() const
{
    return TY_tensor_ety(this);
}


DATA_TYPE Type::getStreamElemType() const
{
    return TY_vec_ety(this);
}


void Type::dump(TypeMgr const* tm) const
{
    tm->dump_type(this);
}


UINT Type::getVectorElemNum(TypeMgr const* tm) const
{
    return TY_vec_size(this) / tm->getDTypeByteSize(TY_vec_ety(this));
}
//END Type


//
//START TensorType
//
Type const* TensorType::getElemType(TypeMgr const* tm) const
{
    return tm->getSimplexTypeEx(tensor_elem_type);
}
//END TensorType

} //namespace xoc
#endif
