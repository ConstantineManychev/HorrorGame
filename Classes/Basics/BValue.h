#ifndef BValue_H
#define BValue_H

#include "CommonDefines.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "platform/CCPlatformMacros.h"
#include "math/Vec2.h"
#include "math/CCGeometry.h"
#include "base/ccTypes.h"

_CSTART

class BValue;

typedef std::vector<BValue> BValueVector;
typedef std::unordered_map<std::string, BValue> BValueUnMap;
typedef std::map<std::string, BValue> BValueMap;
typedef std::unordered_map<int, BValue> BValueIntUnMap;
typedef std::map<int, BValue> BValueIntMap;

extern const BValueVector BValueVectorNull;
extern const BValueMap BValueMapNull;
extern const BValueUnMap BValueUnMapNull;
extern const BValueIntMap BValueIntMapNull;
extern const BValueIntUnMap BValueIntUnMapNull;
/*
 * This class is provide as a wrapper of basic types, such as int and bool.
 */
class  BValue
{
public:
    /** A predefined BValue that has not value. */
    static const BValue Null;

    /** Default constructor. */
    BValue();
    
    /** Create a BValue by an unsigned char value. */
    /*explicit*/ BValue(unsigned char v);
    
    /** Create a BValue by an integer value. */
    /*explicit*/ BValue(int v);
    
    /** Create a BValue by a float value. */
    /*explicit*/ BValue(float v);
    
    /** Create a BValue by a double value. */
    /*explicit*/ BValue(double v);
    
    /** Create a BValue by a bool value. */
    /*explicit*/ BValue(bool v);
    
    /** Create a BValue by a char pointer. It will copy the chars internally. */
    /*explicit*/ BValue(const char* v);
    
    /** Create a BValue by a string. */
    /*explicit*/ BValue(const std::string& v);
    
    /** Create a BValue by a long double value. */
    /*explicit*/ BValue( const long double& v);
    
    /** Create a BValue by an Vec2 value. */
    /*explicit*/ BValue( const cocos2d::Vec2& v );
    
    /** Create a BValue by an Size value. */
    /*explicit*/ BValue( const cocos2d::Size& v );
    
    /** Create a BValue by an Color3B value. */
    /*explicit*/ BValue( const cocos2d::Color3B& v );
    
    /** Create a BValue by an Color4F value. */
    /*explicit*/ BValue( const cocos2d::Color4F& v );
    
    /** Create a BValue by a BValueVector object. */
    /*explicit*/ BValue(const BValueVector& v);
    /** Create a BValue by a BValueVector object. It will use std::move internally. */
    explicit BValue(BValueVector&& v);
    
    /** Create a BValue by a BValueMap object. */
    /*explicit*/ BValue(const BValueMap& v);
    /** Create a BValue by a BValueMap object. It will use std::move internally. */
    explicit BValue(BValueMap&& v);

	/** Create a BValue by a BValueUnMap object. */
	/*explicit*/ BValue( const BValueUnMap& v );
	/** Create a BValue by a BValueUnMap object. It will use std::move internally. */
	explicit BValue( BValueUnMap&& v );
    
    /** Create a BValue by a BValueIntMap object. */
    /*explicit*/ BValue(const BValueIntMap& v);
    /** Create a BValue by a BValueIntMap object. It will use std::move internally. */
    explicit BValue(BValueIntMap&& v);
    
    /** Create a BValue by a BValueIntUnMap object. */
    explicit BValue(const BValueIntUnMap& v);
    /** Create a BValue by a BValueIntUnMap object. It will use std::move internally. */
    explicit BValue(BValueIntUnMap&& v);

    /** Create a BValue by anthoer BValue object. */
    BValue(const BValue& other);
    /** Create a BValue by a BValue object. It will use std::move internally. */
    BValue(BValue&& other);
    
    /** Destructor. */
    ~BValue();

    /** Assignment operator, assign from BValue to BValue. */
    BValue& operator= (const BValue& other);
    /** Assignment operator, assign from BValue to BValue. It will use std::move internally. */
    BValue& operator= (BValue&& other);

    /** Assignment operator, assign from unsigned char to BValue. */
    BValue& operator= (unsigned char v);
    /** Assignment operator, assign from integer to BValue. */
    BValue& operator= (int v);
    /** Assignment operator, assign from float to BValue. */
    BValue& operator= (float v);
    /** Assignment operator, assign from double to BValue. */
    BValue& operator= (double v);
    /** Assignment operator, assign from bool to BValue. */
    BValue& operator= (bool v);
    /** Assignment operator, assign from char* to BValue. */
    BValue& operator= (const char* v);
    /** Assignment operator, assign from string to BValue. */
    BValue& operator= (const std::string& v);
    /** Assignment operator, assign from long double to BValue. */
    BValue& operator= ( const long double& v);
    
    /** Assignment operator, assign from BValueVector to BValue. */
    BValue& operator= (const BValueVector& v);
    /** Assignment operator, assign from BValueVector to BValue. */
    BValue& operator= (BValueVector&& v);
    
    /** Assignment operator, assign from BValueMap to BValue. */
    BValue& operator= (const BValueMap& v);
    /** Assignment operator, assign from BValueMap to BValue. It will use std::move internally. */
    BValue& operator= (BValueMap&& v);

	/** Assignment operator, assign from BValueUnMap to BValue. */
	BValue& operator= ( const BValueUnMap& v );
	/** Assignment operator, assign from BValueUnMap to BValue. It will use std::move internally. */
	BValue& operator= ( BValueUnMap&& v );

    /** Assignment operator, assign from BValueIntMap to BValue. */
    BValue& operator= (const BValueIntMap& v);
    /** Assignment operator, assign from BValueIntMap to BValue. It will use std::move internally. */
    BValue& operator= (BValueIntMap&& v);
    
    /** Assignment operator, assign from BValueIntUnMap to BValue. */
    BValue& operator= (const BValueIntUnMap& v);
    /** Assignment operator, assign from BValueIntUnMap to BValue. It will use std::move internally. */
    BValue& operator= (BValueIntUnMap&& v);

    /** != operator overloading */
    bool operator!= (const BValue& v);
    /** != operator overloading */
    bool operator!= (const BValue& v) const;
    /** == operator overloading */
    bool operator== (const BValue& v);
    /** == operator overloading */
    bool operator== (const BValue& v) const;

    /** Gets as a byte value. Will convert to unsigned char if possible, or will trigger assert error. */
    unsigned char getByte() const;
    /** Gets as an integer value. Will convert to integer if possible, or will trigger assert error. */
    int getInt() const;
    /** Gets as a float value. Will convert to float if possible, or will trigger assert error. */
    float getFloat() const;
    /** Gets as a double value. Will convert to double if possible, or will trigger assert error. */
    double getDouble() const;
    /** Gets as a bool value. Will convert to bool if possible, or will trigger assert error. */
    bool getBool() const;
    /** Gets as a string value. Will convert to string if possible, or will trigger assert error. */
    std::string getString() const;
    
    const long double& getLongDouble() const;
    
    const cocos2d::Vec2& getVec2() const;
    const cocos2d::Size& getSize() const;
    const cocos2d::Color3B& getColor3B() const;
    const cocos2d::Color4F& getColor4F() const;

    std::string getStringForVisit() const;
    /** Gets as a BValueVector reference. Will convert to BValueVector if possible, or will trigger assert error. */
    BValueVector& getValueVector();
    /** Gets as a const BValueVector reference. Will convert to BValueVector if possible, or will trigger assert error. */
    const BValueVector& getValueVector() const;

    /** Gets as a BValueMap reference. Will convert to BValueMap if possible, or will trigger assert error. */
    BValueMap& getValueMap();
    /** Gets as a const BValueMap reference. Will convert to BValueMap if possible, or will trigger assert error. */
    const BValueMap& getValueMap() const;

	/** Gets as a BValueUnMap reference. Will convert to BValueUnMap if possible, or will trigger assert error. */
	BValueUnMap& getValueUnMap();
	/** Gets as a const BValueUnMap reference. Will convert to BValueUnMap if possible, or will trigger assert error. */
	const BValueUnMap& getValueUnMap() const;

    /** Gets as a BValueIntMap reference. Will convert to BValueIntMap if possible, or will trigger assert error. */
    BValueIntMap& getValueIntMap();
    /** Gets as a const BValueIntMap reference. Will convert to BValueIntMap if possible, or will trigger assert error. */
    const BValueIntMap& getValueIntMap() const;
    
    /** Gets as a BValueIntUnMap reference. Will convert to BValueIntUnMap if possible, or will trigger assert error. */
    BValueIntUnMap& getValueIntUnMap();
    /** Gets as a const BValueIntUnMap reference. Will convert to BValueIntUnMap if possible, or will trigger assert error. */
    const BValueIntUnMap& getValueIntUnMap() const;

    /**
     * Checks if the BValue is null.
     * @return True if the BValue is null, false if not.
     */
    inline bool isNull() const { return _type == Type::NONE; }

	inline bool isBoolean()		const { return _type == Type::BOOLEAN; }
	inline bool isByte()		const { return _type == Type::BYTE; }
	inline bool isColor3B()		const { return _type == Type::COLOR3B; }
	inline bool isColor4F()		const { return _type == Type::COLOR4F; }
	inline bool isDouble()		const { return _type == Type::DOUBLE; }
	inline bool isFloat()		const { return _type == Type::FLOAT; }
	inline bool isInteger()		const { return _type == Type::INTEGER; }
	inline bool isIntMap()		const { return _type == Type::INT_MAP; }
	inline bool isIntUnMap()	const { return _type == Type::INT_UN_MAP; }
	inline bool isLongDouble()	const { return _type == Type::LONG_DOUBLE; }
	inline bool isMap()			const { return _type == Type::MAP; }
	inline bool isString()		const { return _type == Type::STRING; }
	inline bool isUnMap()		const { return _type == Type::UN_MAP; }
	inline bool isVec2()		const { return _type == Type::VEC2; }
	inline bool isVector()		const { return _type == Type::VECTOR; }

    /** BValue type wrapped by BValue. */
    enum class Type
    {
        /// no value is wrapped, an empty BValue
        NONE = 0,
        /// wrap byte
        BYTE,
        /// wrap integer
        INTEGER,
        /// wrap float
        FLOAT,
        /// wrap double
        DOUBLE,
        /// wrap bool
        BOOLEAN,
        /// wrap string
        STRING,
        /// wrap vector
        VECTOR,
        /// wrap BValueMap
        MAP,
		/// wrap BValueUnMap
		UN_MAP,
        /// wrap BValueIntMap
        INT_MAP,
        /// wrap BValueIntUnMap
        INT_UN_MAP,
        /// wrap vec2
        VEC2,
        /// wrap size
        SIZE,
        /// wrap color3b
        COLOR3B,
        /// wrap color4f
        COLOR4F,
        /// wrap double
        LONG_DOUBLE,
    };

    /** Gets the value type. */
    inline Type getType() const { return _type; }
    bool checkType( Type type ) const;

    /** Gets is Simple Type or not */
    bool isSimpleType() const;
    /** Gets the description of the class. */
    std::string getDescription() const;

    /** init a string with format, it's similar with the c function 'sprintf'
      convert value type to STRING
     */
    bool initWithFormat(const char* format, ...) CC_FORMAT_PRINTF(2, 3);
    
    /** clear data and set type */
    void clearWithType( Type type );
private:
    
    bool initWithFormatAndValist(const char* format, va_list ap);
    
    void clear();
    void reset(Type type, bool force = false);

    union
    {
        unsigned char byteVal;
        int intVal;
        float floatVal;
        double doubleVal;
        bool boolVal;

        long double* longDoubleVal;
		cocos2d::Vec2* vec2Val;
		cocos2d::Size* sizeVal;
		cocos2d::Color3B* color3BVal;
		cocos2d::Color4F* color4FVal;
        std::string* strVal;
        BValueVector* vectorVal;
        BValueMap* mapVal;
		BValueUnMap* unMapVal;
        BValueIntMap* intMapVal;
        BValueIntUnMap* intUnMapVal;
    }_field;

    Type _type;
};

_CEND


#endif
