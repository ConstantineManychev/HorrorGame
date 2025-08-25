#include "BValue.h"
#include <sstream>
#include <iomanip>
#include "base/ccUtils.h"

_USEC
#define kMaxStringLen (1024*100)

const BValueVector BValueVectorNull;
const BValueMap BValueMapNull;
const BValueUnMap BValueUnMapNull;
const BValueIntMap BValueIntMapNull;
const BValueIntUnMap BValueIntUnMapNull;
const BValue BValue::Null;

static long double g_longDouble = 0.0;
static Vec2 g_printVec2 = Vec2::ZERO;
static Size g_printSize = Size::ZERO;
static Color3B g_printColor3B = Color3B::BLACK;
static Color4F g_printColor4F = Color4F::BLACK;

BValue::BValue()
: _type(Type::NONE)
{
    memset(&_field, 0, sizeof(_field));
}

BValue::BValue(unsigned char v)
: _type(Type::BYTE)
{
    _field.byteVal = v;
}

BValue::BValue(int v)
: _type(Type::INTEGER)
{
    _field.intVal = v;
}

BValue::BValue(float v)
: _type(Type::FLOAT)
{
    _field.floatVal = v;
}

BValue::BValue(double v)
: _type(Type::DOUBLE)
{
    _field.doubleVal = v;
}

BValue::BValue(bool v)
: _type(Type::BOOLEAN)
{
    _field.boolVal = v;
}

BValue::BValue(const char* v)
: _type(Type::STRING)
{
    _field.strVal = new std::string();
    if (v)
    {
        *_field.strVal = v;
    }
}

BValue::BValue(const std::string& v)
: _type(Type::STRING)
{
    _field.strVal = new std::string();
    *_field.strVal = v;
}
BValue::BValue( const long double& v)
: _type(Type::LONG_DOUBLE )
{
    _field.longDoubleVal = new long double;
    *_field.longDoubleVal = v;
}
BValue::BValue( const Vec2& v )
: _type(Type::VEC2)
{
    _field.vec2Val = new Vec2();
    *_field.vec2Val = v;
}
BValue::BValue( const Size& v )
: _type(Type::SIZE)
{
    _field.sizeVal = new Size();
    *_field.sizeVal = v;
}
BValue::BValue( const Color3B& v )
: _type( Type::COLOR3B )
{
    _field.color3BVal = new Color3B();
    *_field.color3BVal = v;
}
BValue::BValue( const Color4F& v )
: _type( Type::COLOR4F )
{
    _field.color4FVal = new Color4F();
    *_field.color4FVal = v;
}
BValue::BValue(const BValueVector& v )
: _type(Type::VECTOR)
{
    _field.vectorVal = new (std::nothrow) BValueVector();
    *_field.vectorVal = v;
}

BValue::BValue(BValueVector&& v)
: _type(Type::VECTOR)
{
    _field.vectorVal = new (std::nothrow) BValueVector();
    *_field.vectorVal = std::move(v);
}

BValue::BValue(const BValueMap& v)
: _type(Type::MAP)
{
    _field.mapVal = new (std::nothrow) BValueMap();
    *_field.mapVal = v;
}

BValue::BValue(BValueMap&& v)
: _type(Type::MAP)
{
    _field.mapVal = new (std::nothrow) BValueMap();
    *_field.mapVal = std::move(v);
}

BValue::BValue( const BValueUnMap& v )
	: _type( Type::UN_MAP )
{
	_field.unMapVal = new ( std::nothrow ) BValueUnMap();
	*_field.unMapVal = v;
}

BValue::BValue( BValueUnMap&& v )
	: _type( Type::UN_MAP )
{
	_field.unMapVal = new ( std::nothrow ) BValueUnMap();
	*_field.unMapVal = std::move( v );
}

BValue::BValue(const BValueIntMap& v)
: _type(Type::INT_MAP)
{
    _field.intMapVal = new (std::nothrow) BValueIntMap();
    *_field.intMapVal = v;
}

BValue::BValue(BValueIntMap&& v)
: _type(Type::INT_MAP)
{
    _field.intMapVal = new (std::nothrow) BValueIntMap();
    *_field.intMapVal = std::move(v);
}
BValue::BValue(const BValueIntUnMap& v)
: _type(Type::INT_UN_MAP)
{
    _field.intUnMapVal = new (std::nothrow) BValueIntUnMap();
    *_field.intUnMapVal = v;
}

BValue::BValue(BValueIntUnMap&& v)
: _type(Type::INT_UN_MAP)
{
    _field.intUnMapVal = new (std::nothrow) BValueIntUnMap();
    *_field.intUnMapVal = std::move(v);
}
BValue::BValue(const BValue& other)
: _type(Type::NONE)
{
    *this = other;
}

BValue::BValue(BValue&& other)
: _type(Type::NONE)
{
    *this = std::move(other);
}

BValue::~BValue()
{
    clear();
}

BValue& BValue::operator= (const BValue& other)
{
    if (this != &other) {
        reset(other._type);

        switch (other._type) {
            case Type::BYTE:
                _field.byteVal = other._field.byteVal;
                break;
            case Type::INTEGER:
                _field.intVal = other._field.intVal;
                break;
            case Type::FLOAT:
                _field.floatVal = other._field.floatVal;
                break;
            case Type::DOUBLE:
                _field.doubleVal = other._field.doubleVal;
                break;
            case Type::BOOLEAN:
                _field.boolVal = other._field.boolVal;
                break;
            case Type::LONG_DOUBLE:
                if (_field.longDoubleVal == nullptr)
                {
                    _field.longDoubleVal = new long double;
                }
                *_field.longDoubleVal = *other._field.longDoubleVal;
                break;
            case Type::STRING:
                if (_field.strVal == nullptr)
                {
                    _field.strVal = new std::string();
                }
                *_field.strVal = *other._field.strVal;
                break;
            case Type::VEC2:
                if (_field.vec2Val == nullptr)
                {
                    _field.vec2Val = new Vec2();
                }
                *_field.vec2Val = *other._field.vec2Val;
                break;
            case Type::SIZE:
                if (_field.sizeVal == nullptr)
                {
                    _field.sizeVal = new Size();
                }
                *_field.sizeVal = *other._field.sizeVal;
                break;
            case Type::COLOR3B:
                if (_field.color3BVal == nullptr)
                {
                    _field.color3BVal = new Color3B();
                }
                *_field.color3BVal = *other._field.color3BVal;
                break;
            case Type::COLOR4F:
                if (_field.color4FVal == nullptr)
                {
                    _field.color4FVal = new Color4F();
                }
                *_field.color4FVal = *other._field.color4FVal;
                break;
            case Type::VECTOR:
                if (_field.vectorVal == nullptr)
                {
                    _field.vectorVal = new (std::nothrow) BValueVector();
                }
                *_field.vectorVal = *other._field.vectorVal;
                break;
            case Type::MAP:
                if (_field.mapVal == nullptr)
                {
                    _field.mapVal = new (std::nothrow) BValueMap();
                }
                *_field.mapVal = *other._field.mapVal;
                break;
            case Type::UN_MAP:
                if (_field.unMapVal == nullptr)
                {
                    _field.unMapVal = new (std::nothrow) BValueUnMap();
                }
                *_field.unMapVal = *other._field.unMapVal;
            
                break;
            case Type::INT_MAP:
                if (_field.intMapVal == nullptr)
                {
                    _field.intMapVal = new (std::nothrow) BValueIntMap();
                }
                *_field.intMapVal = *other._field.intMapVal;
            
            break;
            case Type::INT_UN_MAP:
                if (_field.intUnMapVal == nullptr)
                {
                    _field.intUnMapVal = new (std::nothrow) BValueIntUnMap();
                }
                *_field.intUnMapVal = *other._field.intUnMapVal;
            
            break;
            default:
                break;
        }
    }
    return *this;
}

BValue& BValue::operator= (BValue&& other)
{
    if (this != &other)
    {
        clear();
        switch (other._type)
        {
            case Type::BYTE:
                _field.byteVal = other._field.byteVal;
                break;
            case Type::INTEGER:
                _field.intVal = other._field.intVal;
                break;
            case Type::FLOAT:
                _field.floatVal = other._field.floatVal;
                break;
            case Type::DOUBLE:
                _field.doubleVal = other._field.doubleVal;
                break;
            case Type::BOOLEAN:
                _field.boolVal = other._field.boolVal;
                break;
            case Type::STRING:
                _field.strVal = other._field.strVal;
                break;
            case Type::LONG_DOUBLE:
                _field.longDoubleVal = other._field.longDoubleVal;
                break;
            case Type::VEC2:
                _field.vec2Val = other._field.vec2Val;
                break;
            case Type::SIZE:
                _field.sizeVal = other._field.sizeVal;
                break;
            case Type::COLOR3B:
                _field.color3BVal = other._field.color3BVal;
                break;
            case Type::COLOR4F:
                _field.color4FVal = other._field.color4FVal;
                break;
            case Type::VECTOR:
                _field.vectorVal = other._field.vectorVal;
                break;
            case Type::MAP:
                _field.mapVal = other._field.mapVal;
                break;
            case Type::UN_MAP:
                _field.unMapVal = other._field.unMapVal;
                break;
            case Type::INT_MAP:
                _field.intMapVal = other._field.intMapVal;
            break;
            case Type::INT_UN_MAP:
                _field.intUnMapVal = other._field.intUnMapVal;
            break;
            default:
                break;
        }
        _type = other._type;

        memset(&other._field, 0, sizeof(other._field));
        other._type = Type::NONE;
    }

    return *this;
}

BValue& BValue::operator= (unsigned char v)
{
    reset(Type::BYTE);
    _field.byteVal = v;
    return *this;
}

BValue& BValue::operator= (int v)
{
    reset(Type::INTEGER);
    _field.intVal = v;
    return *this;
}

BValue& BValue::operator= (float v)
{
    reset(Type::FLOAT);
    _field.floatVal = v;
    return *this;
}

BValue& BValue::operator= (double v)
{
    reset(Type::DOUBLE);
    _field.doubleVal = v;
    return *this;
}

BValue& BValue::operator= (bool v)
{
    reset(Type::BOOLEAN);
    _field.boolVal = v;
    return *this;
}

BValue& BValue::operator= (const char* v)
{
    reset(Type::STRING);
    *_field.strVal = v ? v : "";
    return *this;
}

BValue& BValue::operator= (const std::string& v)
{
    reset(Type::STRING);
    *_field.strVal = v;
    return *this;
}

BValue& BValue::operator= (const long double& v)
{
    reset(Type::LONG_DOUBLE);
    *_field.longDoubleVal = v;
    return *this;
}
BValue& BValue::operator= (const BValueVector& v)
{
    reset(Type::VECTOR);
    *_field.vectorVal = v;
    return *this;
}

BValue& BValue::operator= (BValueVector&& v)
{
    reset(Type::VECTOR);
    *_field.vectorVal = std::move(v);
    return *this;
}

BValue& BValue::operator= (const BValueMap& v)
{
    reset(Type::MAP);
    *_field.mapVal = v;
    return *this;
}

BValue& BValue::operator= (BValueMap&& v)
{
    reset(Type::MAP);
    *_field.mapVal = std::move(v);
    return *this;
}

BValue& BValue::operator= ( const BValueUnMap& v )
{
    reset( Type::UN_MAP );
	*_field.unMapVal = v;
	return *this;
}

BValue& BValue::operator= ( BValueUnMap&& v )
{
	reset( Type::UN_MAP );
	*_field.unMapVal = std::move( v );
	return *this;
}

BValue& BValue::operator= (const BValueIntMap& v)
{
    reset(Type::INT_MAP);
    *_field.intMapVal = v;
    return *this;
}

BValue& BValue::operator= (BValueIntMap&& v)
{
    reset(Type::INT_MAP);
    *_field.intMapVal = std::move(v);
    return *this;
}
BValue& BValue::operator= (const BValueIntUnMap& v)
{
    reset(Type::INT_UN_MAP);
    *_field.intUnMapVal = v;
    return *this;
}
BValue& BValue::operator= (BValueIntUnMap&& v)
{
    reset(Type::INT_UN_MAP);
    *_field.intUnMapVal = std::move(v);
    return *this;
}

bool BValue::operator!= (const BValue& v)
{
    return !(*this == v);
}
bool BValue::operator!= (const BValue& v) const
{
    return !(*this == v);
}

bool BValue::operator== (const BValue& v)
{
    const auto &t = *this;
    return t == v;
}
bool BValue::operator== (const BValue& v) const
{
    if (this == &v) return true;
    if (v._type != this->_type) return false;
	if ( this->isNull() && v.isNull() ) return true;
	if ( this->isNull() ) return false;
	if ( v.isNull() ) return false;

    switch (_type)
    {
    case Type::BYTE:    return v._field.byteVal   == this->_field.byteVal;
    case Type::INTEGER: return v._field.intVal    == this->_field.intVal;
    case Type::BOOLEAN: return v._field.boolVal   == this->_field.boolVal;
    case Type::STRING:  return *v._field.strVal   == *this->_field.strVal;
    case Type::LONG_DOUBLE:  return fabs( (*v._field.longDoubleVal) - (*this->_field.longDoubleVal) ) <= 0.000001f;//FLT_EPSILON;
    case Type::VEC2:  return this->_field.vec2Val->equals( *v._field.vec2Val );
    case Type::SIZE:  return this->_field.sizeVal->equals( *v._field.sizeVal );
    case Type::COLOR3B:  return this->_field.color3BVal->equals( *v._field.color3BVal );
    case Type::COLOR4F:  return this->_field.color4FVal->equals( *v._field.color4FVal );
    case Type::FLOAT:   return fabs(v._field.floatVal  - this->_field.floatVal)  <= 0.000001f;//FLT_EPSILON;
    case Type::DOUBLE:  return fabs(v._field.doubleVal - this->_field.doubleVal) <= 0.000001f;//FLT_EPSILON;
    case Type::VECTOR:
    {
        const auto &v1 = *(this->_field.vectorVal);
        const auto &v2 = *(v._field.vectorVal);
        const auto size = v1.size();
        if (size == v2.size())
        {
            for (size_t i = 0; i < size; i++)
            {
                if (v1[i] != v2[i]) return false;
            }

			return true;
        }
        return false;
    }
    case Type::MAP:
    {
        const auto &map1 = *(this->_field.mapVal);
        const auto &map2 = *(v._field.mapVal);
		if ( map1.size() != map2.size() )
		{
			return false;
		}
        for (const auto &kvp : map1)
        {
            auto it = map2.find(kvp.first);
            if (it == map2.end() || it->second != kvp.second)
            {
                return false;
            }
        }
        return true;
    }
    case Type::UN_MAP:
    {
        const auto &map1 = *(this->_field.unMapVal);
        const auto &map2 = *(v._field.unMapVal);
		if ( map1.size() != map2.size() )
		{
			return false;
		}
        for (const auto &kvp : map1)
        {
            auto it = map2.find(kvp.first);
            if (it == map2.end() || it->second != kvp.second)
            {
                return false;
            }
        }
        return true;
    }
    case Type::INT_MAP:
    {
        const auto &map1 = *(this->_field.intMapVal);
        const auto &map2 = *(v._field.intMapVal);
		if ( map1.size() != map2.size() )
		{
			return false;
		}
        for (const auto &kvp : map1)
        {
            auto it = map2.find(kvp.first);
            if (it == map2.end() || it->second != kvp.second)
            {
                return false;
            }
        }
        return true;
    }
        
    case Type::INT_UN_MAP:
    {
        const auto &map1 = *(this->_field.intUnMapVal);
        const auto &map2 = *(v._field.intUnMapVal);
		if ( map1.size() != map2.size() )
		{
			return false;
		}
        for (const auto &kvp : map1)
        {
            auto it = map2.find(kvp.first);
            if (it == map2.end() || it->second != kvp.second)
            {
                return false;
            }
        }
        return true;
    }
        
    default:
        break;
    };

    return false;
}

/// Convert value to a specified type
unsigned char BValue::getByte() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP , "Only base type (bool, string, float, double, int) could be converted");

    if (_type == Type::BYTE)
    {
        return _field.byteVal;
    }

    if (_type == Type::INTEGER)
    {
        return static_cast<unsigned char>(_field.intVal);
    }

    if (_type == Type::STRING)
    {
        return static_cast<unsigned char>(atoi(_field.strVal->c_str()));
    }
    if (_type == Type::LONG_DOUBLE)
    {
        return static_cast<unsigned char>( *_field.longDoubleVal );
    }
    if (_type == Type::VEC2)
    {
        return static_cast<unsigned char>( _field.vec2Val->length() );
    }
    
    if (_type == Type::SIZE)
    {
        return static_cast<unsigned char>( _field.sizeVal->width );
    }
    
    if (_type == Type::COLOR3B)
    {
        return static_cast<unsigned char>( _field.color3BVal->r );
    }
    
    if (_type == Type::COLOR4F)
    {
        return static_cast<unsigned char>( _field.color4FVal->r );
    }

    if (_type == Type::FLOAT)
    {
        return static_cast<unsigned char>(_field.floatVal);
    }

    if (_type == Type::DOUBLE)
    {
        return static_cast<unsigned char>(_field.doubleVal);
    }

    if (_type == Type::BOOLEAN)
    {
        return _field.boolVal ? 1 : 0;
    }

    return 0;
}

int BValue::getInt() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    if (_type == Type::INTEGER)
    {
        return _field.intVal;
    }

    if (_type == Type::BYTE)
    {
        return _field.byteVal;
    }

    if (_type == Type::STRING)
    {
        return atoi(_field.strVal->c_str());
    }
    if (_type == Type::LONG_DOUBLE)
    {
        return static_cast<int>( *_field.longDoubleVal );
    }
    if (_type == Type::VEC2)
    {
        return static_cast<int>(_field.vec2Val->length());
    }
    
    if (_type == Type::SIZE)
    {
        return static_cast<int>(_field.sizeVal->width );
    }
    
    if (_type == Type::COLOR3B)
    {
        return static_cast<int>(_field.color3BVal->r);
    }
    
    if (_type == Type::COLOR4F)
    {
        return static_cast<int>(_field.color4FVal->r);
    }
    
    if (_type == Type::FLOAT)
    {
        return static_cast<int>(_field.floatVal);
    }

    if (_type == Type::DOUBLE)
    {
        return static_cast<int>(_field.doubleVal);
    }

    if (_type == Type::BOOLEAN)
    {
        return _field.boolVal ? 1 : 0;
    }

    return 0;
}

float BValue::getFloat() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    if (_type == Type::FLOAT)
    {
        return _field.floatVal;
    }

    if (_type == Type::BYTE)
    {
        return static_cast<float>(_field.byteVal);
    }

    if (_type == Type::STRING)
    {
        return static_cast<float>(utils::atof(_field.strVal->c_str()));
    }
    if (_type == Type::LONG_DOUBLE)
    {
        return static_cast<float>( *_field.longDoubleVal );
    }
    if (_type == Type::VEC2)
    {
        return _field.vec2Val->length();
    }
    
    if (_type == Type::SIZE)
    {
        return _field.sizeVal->width;
    }
    
    if (_type == Type::COLOR3B)
    {
        return _field.color3BVal->r;
    }
    
    if (_type == Type::COLOR4F)
    {
        return _field.color4FVal->r;
    }

    if (_type == Type::INTEGER)
    {
        return static_cast<float>(_field.intVal);
    }

    if (_type == Type::DOUBLE)
    {
        return static_cast<float>(_field.doubleVal);
    }

    if (_type == Type::BOOLEAN)
    {
        return _field.boolVal ? 1.0f : 0.0f;
    }

    return 0.0f;
}

double BValue::getDouble() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    if (_type == Type::DOUBLE)
    {
        return _field.doubleVal;
    }

    if (_type == Type::BYTE)
    {
        return static_cast<double>(_field.byteVal);
    }

    if (_type == Type::STRING)
    {
        return static_cast<double>(utils::atof(_field.strVal->c_str()));
    }
    if (_type == Type::LONG_DOUBLE)
    {
        return static_cast<double>( *_field.longDoubleVal );
    }
    if (_type == Type::VEC2)
    {
        return static_cast<double>( _field.vec2Val->length());
    }
    
    if (_type == Type::SIZE)
    {
        return static_cast<double>( _field.sizeVal->width );
    }
    if (_type == Type::COLOR3B)
    {
        return static_cast<double>( _field.color3BVal->r );
    }
    if (_type == Type::COLOR4F)
    {
        return static_cast<double>( _field.color4FVal->r );
    }
    
    if (_type == Type::INTEGER)
    {
        return static_cast<double>(_field.intVal);
    }

    if (_type == Type::FLOAT)
    {
        return static_cast<double>(_field.floatVal);
    }

    if (_type == Type::BOOLEAN)
    {
        return _field.boolVal ? 1.0 : 0.0;
    }

    return 0.0;
}

bool BValue::getBool() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    if (_type == Type::BOOLEAN)
    {
        return _field.boolVal;
    }

    if (_type == Type::BYTE)
    {
        return _field.byteVal == 0 ? false : true;
    }

    if (_type == Type::STRING)
    {
        return (*_field.strVal == "0" || *_field.strVal == "false") ? false : true;
    }
    if (_type == Type::LONG_DOUBLE)
    {
        return *_field.longDoubleVal == 0.0f ? false : true;
    }
    if (_type == Type::VEC2)
    {
        return _field.vec2Val->length() == 0.0f ? false : true;
    }
    
    if (_type == Type::SIZE)
    {
        return _field.sizeVal->width == 0.0f ? false : true;
    }
    
    if (_type == Type::COLOR3B)
    {
        return _field.color3BVal->r == 0.0f ? false : true;
    }
    
    if (_type == Type::COLOR4F)
    {
        return _field.color4FVal->r == 0.0f ? false : true;
    }
    
    if (_type == Type::INTEGER)
    {
        return _field.intVal == 0 ? false : true;
    }

    if (_type == Type::FLOAT)
    {
        return _field.floatVal == 0.0f ? false : true;
    }

    if (_type == Type::DOUBLE)
    {
        return _field.doubleVal == 0.0 ? false : true;
    }

    return false;
}

std::string BValue::getString() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");

    if (_type == Type::STRING)
    {
        return *_field.strVal;
    }

    std::stringstream ret;

    switch (_type)
    {
        case Type::BYTE:
            ret << _field.byteVal;
            break;
        case Type::INTEGER:
            ret << _field.intVal;
            break;
        case Type::FLOAT:
            ret << std::fixed << std::setprecision( 7 )<< _field.floatVal;
            break;
        case Type::VEC2:
            ret << std::fixed << std::setprecision( 7 ) << _field.vec2Val->x << "," << _field.vec2Val->y;
            break;
        case Type::SIZE:
            ret << std::fixed << std::setprecision( 7 ) << _field.sizeVal->width << "," << _field.sizeVal->height;
            break;
        case Type::COLOR3B:
            ret << std::fixed << std::setprecision( 0 ) << static_cast<int>(_field.color3BVal->r) << "," << static_cast<int>(_field.color3BVal->g) << "," << static_cast<int>(_field.color3BVal->b);
            break;
        case Type::COLOR4F:
            ret << std::fixed << std::setprecision( 7 ) << _field.color4FVal->r << "," << _field.color4FVal->g << "," << _field.color4FVal->b << "," << _field.color4FVal->a;
            break;
        case Type::DOUBLE:
            ret << std::fixed << std::setprecision( 16 ) << _field.doubleVal;
            break;
        case Type::LONG_DOUBLE:
            ret << std::fixed << std::setprecision( 16 ) << *_field.longDoubleVal;
            break;
        case Type::BOOLEAN:
            ret << (_field.boolVal ? "true" : "false");
            break;
        default:
            break;
    }
    return ret.str();
}
const long double& BValue::getLongDouble() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
 
    if (_type == Type::LONG_DOUBLE)
    {
        return *_field.longDoubleVal;
    }
    
    switch (_type)
    {
        case Type::BYTE:
        {
            g_longDouble = static_cast<long double>(_field.byteVal);
            return g_longDouble;
        }
            break;
        case Type::INTEGER:
        {
            g_longDouble = static_cast<long double>(_field.intVal);
            
            return g_longDouble;
        }
            break;
        case Type::FLOAT:
        {
            g_longDouble = static_cast<long double>(_field.floatVal);
            
            return g_longDouble;
        }
            break;
        case Type::STRING:
        {
            g_longDouble = static_cast<long double>(std::stod(_field.strVal->c_str()));
            
            return g_longDouble;
        }
            break;
        case Type::DOUBLE:
        {
            g_longDouble = _field.doubleVal;
            
            return g_longDouble;
        }
            break;
        case Type::BOOLEAN:
        {
            if ( _field.boolVal )
            {
                g_longDouble = 1.0;
            }
            else
            {
                g_longDouble = 0.0;
            }
            
            return g_longDouble;
        }
            break;
        case Type::VEC2:
        {
            g_longDouble = static_cast<long double>(_field.vec2Val->x);
            
            return g_longDouble;
        }
            break;
        case Type::SIZE:
        {
            g_longDouble = static_cast<long double>(_field.sizeVal->width);
            
            return g_longDouble;
        }
            break;
        case Type::COLOR3B:
        {
            g_longDouble = static_cast<long double>(_field.color3BVal->r);
            
            return g_longDouble;
        }
            break;
        case Type::COLOR4F:
        {
            g_longDouble = static_cast<long double>(_field.color4FVal->r);
            
            return g_longDouble;
        }
            break;
        default:
            break;
    }
    g_longDouble = 0.0;
    return g_longDouble;
}
const Vec2& BValue::getVec2() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    
    if (_type == Type::VEC2)
    {
        return *_field.vec2Val;
    }
 
    switch (_type)
    {
        case Type::BYTE:
        {
            g_printVec2.x = static_cast<float>(_field.byteVal);
            g_printVec2.y = 0.0f;
            
            return g_printVec2;
        }
            break;
        case Type::INTEGER:
        {
            g_printVec2.x = static_cast<float>(_field.intVal);
            g_printVec2.y = 0.0f;
            
            return g_printVec2;
        }
            break;
        case Type::FLOAT:
        {
            g_printVec2.x = _field.floatVal;
            g_printVec2.y = 0.0f;
            
            return g_printVec2;
        }
            break;
        case Type::STRING:
        {
            g_printVec2 = Vec2::ZERO;
            
            auto pos = _field.strVal->find( ',' );
            if ( pos != std::string::npos )
            {
                g_printVec2.x = static_cast<float>(utils::atof(_field.strVal->substr( 0, pos ).c_str()));
                g_printVec2.y = static_cast<float>(utils::atof(_field.strVal->substr( pos + 1, _field.strVal->size() - pos - 1 ).c_str()));
            }
            
            return g_printVec2;
        }
            break;
        case Type::LONG_DOUBLE:
        {
            g_printVec2.x = static_cast<float>(*_field.longDoubleVal);
            g_printVec2.y = 0.0f;
            
            return g_printVec2;
        }
            break;
        case Type::DOUBLE:
        {
            g_printVec2.x = static_cast<float>(_field.doubleVal);
            g_printVec2.y = 0.0f;
            
            return g_printVec2;
        }
            break;
        case Type::BOOLEAN:
        {
            if ( _field.boolVal )
            {
                g_printVec2.x = 1.0f;
                g_printVec2.y = 1.0f;
            }
            else
            {
                g_printVec2 = Vec2::ZERO;
            }
            return g_printVec2;
        }
            break;
        case Type::SIZE:
        {
            g_printVec2.x = static_cast<float>(_field.sizeVal->width);
            g_printVec2.y = static_cast<float>(_field.sizeVal->height);
            
            return g_printVec2;
        }
            break;
        case Type::COLOR3B:
        {
            g_printVec2.x = static_cast<float>(_field.color3BVal->r);
            g_printVec2.y = static_cast<float>(_field.color3BVal->g);
            
            return g_printVec2;
        }
            break;
        case Type::COLOR4F:
        {
            g_printVec2.x = static_cast<float>(_field.color4FVal->r);
            g_printVec2.y = static_cast<float>(_field.color4FVal->g);
            
            return g_printVec2;
        }
            break;
        default:
            break;
    }
    
    return Vec2::ZERO;
}
const Size& BValue::getSize() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    
    if (_type == Type::SIZE)
    {
        return *_field.sizeVal;
    }
    
    switch (_type)
    {
        case Type::BYTE:
        {
            g_printSize.width = static_cast<float>(_field.byteVal);
            g_printSize.height = 0.0f;
            
            return g_printSize;
        }
            break;
        case Type::INTEGER:
        {
            g_printSize.width = static_cast<float>(_field.intVal);
            g_printSize.height = 0.0f;
            
            return g_printSize;
        }
            break;
        case Type::FLOAT:
        {
            g_printSize.width = _field.floatVal;
            g_printSize.height = 0.0f;
            
            return g_printSize;
        }
            break;
        case Type::STRING:
        {
            g_printSize = Size::ZERO;
            
            auto pos = _field.strVal->find( ',' );
            if ( pos != std::string::npos )
            {
                g_printSize.width = static_cast<float>(utils::atof(_field.strVal->substr( 0, pos ).c_str()));
                g_printSize.height = static_cast<float>(utils::atof(_field.strVal->substr( pos + 1, _field.strVal->size() - pos - 1 ).c_str()));
            }
            
            return g_printSize;
        }
            break;
        case Type::LONG_DOUBLE:
        {
            g_printSize.width = static_cast<float>(*_field.longDoubleVal);
            g_printSize.height = 0.0f;
            
            return g_printSize;
        }
            break;
        case Type::DOUBLE:
        {
            g_printSize.width = static_cast<float>(_field.doubleVal);
            g_printSize.height = 0.0f;
            
            return g_printSize;
        }
            break;
        case Type::BOOLEAN:
        {
            if ( _field.boolVal )
            {
                g_printSize.width = 1.0f;
                g_printSize.height = 1.0f;
            }
            else
            {
                g_printSize = Size::ZERO;
            }
            return g_printSize;
        }
            break;
        case Type::VEC2:
        {
            g_printSize.width = static_cast<float>(_field.vec2Val->x);
            g_printSize.height = static_cast<float>(_field.vec2Val->y);
            
            return g_printSize;
        }
            break;
        case Type::COLOR3B:
        {
            g_printSize.width = static_cast<float>(_field.color3BVal->r);
            g_printSize.height = static_cast<float>(_field.color3BVal->g);
            
            return g_printSize;
        }
            break;
        case Type::COLOR4F:
        {
            g_printSize.width = static_cast<float>(_field.color4FVal->r);
            g_printSize.height = static_cast<float>(_field.color4FVal->g);
            
            return g_printSize;
        }
            break;
        default:
            break;
    }
    
    return Size::ZERO;
}
const Color3B& BValue::getColor3B() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    
    if (_type == Type::COLOR3B)
    {
        return *_field.color3BVal;
    }
    
    switch (_type)
    {
        case Type::BYTE:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.byteVal);
            g_printColor3B.g = 0;
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::INTEGER:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.intVal);
            g_printColor3B.g = 0;
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::FLOAT:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.floatVal);
            g_printColor3B.g = 0;
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::STRING:
        {
            g_printColor3B = Color3B::BLACK;
            
            std::string rawStr = *_field.strVal;
        
            auto pos = rawStr.find( ',' );
            
            int i = 0;
            while ( ( !rawStr.empty() ) && ( i < 3 ) )
            {
                
                const int colSeg = atoi( rawStr.substr( 0, pos ).c_str() );
                rawStr = rawStr.substr( pos + 1, rawStr.size() - pos - 1 );
                
                switch ( i ) {
                    case 0:
                    {
                        g_printColor3B.r = static_cast<uint8_t>(colSeg);
                    }
                        break;
                    case 1:
                    {
                        g_printColor3B.g = static_cast<uint8_t>(colSeg);
                    }
                        break;
                    case 2:
                    {
                        g_printColor3B.b = static_cast<uint8_t>(colSeg);
                    }
                        break;
                    default:
                        break;
                }
                
                pos = rawStr.find( ',' );
                if ( pos == std::string::npos )
                {
					pos = rawStr.size();
					rawStr += ',';
                }
                i++;
            }
            
            return g_printColor3B;
        }
            break;
        case Type::LONG_DOUBLE:
        {
            g_printColor3B.r = static_cast<uint8_t>(*_field.longDoubleVal);
            g_printColor3B.g = 0;
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::DOUBLE:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.doubleVal);
            g_printColor3B.g = 0;
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::BOOLEAN:
        {
            if ( _field.boolVal )
            {
                g_printColor3B = Color3B::WHITE;
            }
            else
            {
                g_printColor3B = Color3B::BLACK;
            }
            return g_printColor3B;
        }
            break;
        case Type::VEC2:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.vec2Val->x);
            g_printColor3B.g = static_cast<uint8_t>(_field.vec2Val->y);
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::SIZE:
        {
            g_printColor3B.r = static_cast<uint8_t>(_field.sizeVal->width);
            g_printColor3B.g = static_cast<uint8_t>(_field.sizeVal->height);
            g_printColor3B.b = 0;
            
            return g_printColor3B;
        }
            break;
        case Type::COLOR4F:
        {
            g_printColor3B = Color3B( *_field.color4FVal );
            
            return g_printColor3B;
        }
            break;
        default:
            break;
    }
    
    return Color3B::BLACK;
}
const Color4F& BValue::getColor4F() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    
    if (_type == Type::COLOR4F)
    {
        return *_field.color4FVal;
    }
    
    switch (_type)
    {
        case Type::BYTE:
        {
            g_printColor4F.r = static_cast<float>(_field.byteVal);
            g_printColor4F.g = 0.0f;
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::INTEGER:
        {
            g_printColor4F.r = static_cast<float>(_field.intVal);
            g_printColor4F.g = 0.0f;
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::FLOAT:
        {
            g_printColor4F.r = _field.floatVal;
            g_printColor4F.g = 0.0f;
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::STRING:
        {
            g_printColor4F = Color4F::BLACK;
            
            std::string rawStr = *_field.strVal;
            
            auto pos = rawStr.find( ',' );
            
            int i = 0;
            while ( ( !rawStr.empty() ) && ( i < 4 ) )
            {
                const float colSeg = static_cast<float>(utils::atof( rawStr.substr( 0, pos ).c_str() ));
                rawStr = rawStr.substr( pos + 1, rawStr.size() - pos - 1 );
                
                switch ( i ) {
                    case 0:
                    {
                        g_printColor4F.r = colSeg;
                    }
                        break;
                    case 1:
                    {
                        g_printColor4F.g = colSeg;
                    }
                        break;
                    case 2:
                    {
                        g_printColor4F.b = colSeg;
                    }
                        break;
                    case 3:
                    {
                        g_printColor4F.a = colSeg;
                    }
                        break;
                    default:
                        break;
                }
                
                pos = rawStr.find( ',' );
                if ( pos == std::string::npos )
                {
					pos = rawStr.size();
					rawStr += ',';
                }
                i++;
            }
            
            return g_printColor4F;
        }
            break;
        case Type::LONG_DOUBLE:
        {
            g_printColor4F.r = static_cast<float>(*_field.longDoubleVal);
            g_printColor4F.g = 0.0f;
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::DOUBLE:
        {
            g_printColor4F.r = static_cast<float>(_field.doubleVal);
            g_printColor4F.g = 0.0f;
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::BOOLEAN:
        {
            if ( _field.boolVal )
            {
                g_printColor4F = Color4F::WHITE;
            }
            else
            {
                g_printColor4F = Color4F::BLACK;
            }
            return g_printColor4F;
        }
            break;
        case Type::VEC2:
        {
            g_printColor4F.r = static_cast<float>(_field.vec2Val->x);
            g_printColor4F.g = static_cast<float>(_field.vec2Val->y);
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::SIZE:
        {
            g_printColor4F.r = static_cast<float>(_field.sizeVal->width);
            g_printColor4F.g = static_cast<float>(_field.sizeVal->height);
            g_printColor4F.b = 0.0f;
            g_printColor4F.a = 0.0f;
            
            return g_printColor4F;
        }
            break;
        case Type::COLOR3B:
        {
            g_printColor4F = Color4F( *_field.color3BVal );
            
            return g_printColor4F;
        }
            break;
        default:
            break;
    }
    
    return Color4F::BLACK;
}
std::string BValue::getStringForVisit() const
{
    CCASSERT(_type != Type::VECTOR && _type != Type::MAP && _type != Type::UN_MAP && _type != Type::INT_MAP && _type != Type::INT_UN_MAP, "Only base type (bool, string, float, double, int) could be converted");
    
    if (_type == Type::STRING)
    {
        return *_field.strVal;
    }
    
    std::stringstream ret;
    
    switch (_type)
    {
        case Type::BYTE:
        {
            float fRet = static_cast<float>(_field.byteVal);
            ret << std::fixed << std::setprecision( 7 ) << fRet;
        }
            break;
        case Type::INTEGER:
        {
            float fRet = static_cast<float>(_field.intVal);
            ret << std::fixed << std::setprecision( 7 ) << fRet;
        }
            break;
        case Type::FLOAT:
        {
            float fRet = _field.floatVal;
            ret << std::fixed << std::setprecision( 7 ) << fRet;
        }
            break;
        case Type::DOUBLE:
        {
            float fRet = static_cast<float>(_field.doubleVal);
            ret << std::fixed << std::setprecision( 7 ) << fRet;
        }
            break;
        case Type::LONG_DOUBLE:
        {
            ret << std::fixed << std::setprecision( 16 ) << *_field.longDoubleVal;
        }
            break;
        case Type::VEC2:
        {
            ret << std::fixed << std::setprecision( 7 ) << _field.vec2Val->x << "," << _field.vec2Val->y;
        }
            break;
		case Type::SIZE:
		{
			ret << std::fixed << std::setprecision(7) << _field.sizeVal->width << "," << _field.sizeVal->height;
		}
			break;
		case Type::COLOR3B:
		{
			ret << std::fixed << std::setprecision(0) << static_cast<int>(_field.color3BVal->r) << "," << static_cast<int>(_field.color3BVal->g) << "," << static_cast<int>(_field.color3BVal->b);
		}
			break;
		case Type::COLOR4F:
		{
			ret << std::fixed << std::setprecision(7) << _field.color4FVal->r << "," << _field.color4FVal->g << "," << _field.color4FVal->b << "," << _field.color4FVal->a;
		}
			break;
        case Type::BOOLEAN:
            ret << (_field.boolVal ? "true" : "false");
            break;
        default:
            break;
    }
    return ret.str();
}
BValueVector& BValue::getValueVector()
{
    CCASSERT(_type == Type::VECTOR, "The value type isn't Type::VECTOR");
    return *_field.vectorVal;
}

const BValueVector& BValue::getValueVector() const
{
    CCASSERT(_type == Type::VECTOR, "The value type isn't Type::VECTOR");
    return *_field.vectorVal;
}

BValueMap& BValue::getValueMap()
{
    CCASSERT(_type == Type::MAP, "The value type isn't Type::MAP");
    return *_field.mapVal;
}

const BValueMap& BValue::getValueMap() const
{
    CCASSERT(_type == Type::MAP, "The value type isn't Type::MAP");
    return *_field.mapVal;
}

BValueUnMap& BValue::getValueUnMap()
{
	CCASSERT( _type == Type::UN_MAP, "The value type isn't Type::UN_MAP" );
	return *_field.unMapVal;
}

const BValueUnMap& BValue::getValueUnMap() const
{
	CCASSERT( _type == Type::UN_MAP, "The value type isn't Type::UN_MAP" );
	return *_field.unMapVal;
}

BValueIntMap& BValue::getValueIntMap()
{
    CCASSERT(_type == Type::INT_MAP, "The value type isn't Type::INT_MAP");
    return *_field.intMapVal;
}

const BValueIntMap& BValue::getValueIntMap() const
{
    CCASSERT(_type == Type::INT_MAP, "The value type isn't Type::INT_MAP");
    return *_field.intMapVal;
}

BValueIntUnMap& BValue::getValueIntUnMap()
{
    CCASSERT(_type == Type::INT_UN_MAP, "The value type isn't Type::INT_UN_MAP");
    return *_field.intUnMapVal;
}

const BValueIntUnMap& BValue::getValueIntUnMap() const
{
    CCASSERT(_type == Type::INT_UN_MAP, "The value type isn't Type::INT_UN_MAP");
    return *_field.intUnMapVal;
}

static std::string getTabs(int depth)
{
    std::string tabWidth;

    for (int i = 0; i < depth; ++i)
    {
        tabWidth += "\t";
    }

    return tabWidth;
}

static std::string visit(const BValue& v, int depth);

static std::string visitVector(const BValueVector& v, int depth)
{
    std::stringstream ret;

    if (depth > 0)
        ret << "\n";

    ret << getTabs(depth) << "[\n";

    int i = 0;
    for (const auto& child : v)
    {
        ret << getTabs(depth+1) << i << ": " << visit(child, depth + 1);
        ++i;
    }

    ret << getTabs(depth) << "]\n";

    return ret.str();
}

template <class T>
static std::string visitMap(const T& v, int depth)
{
    std::stringstream ret;

    if (depth > 0)
        ret << "\n";

    ret << getTabs(depth) << "{\n";

    for (auto iter = v.begin(); iter != v.end(); ++iter)
    {
        ret << getTabs(depth + 1) << iter->first << ": ";
        ret << visit(iter->second, depth + 1);
    }

    ret << getTabs(depth) << "}\n";

    return ret.str();
}
static std::string visit(const BValue& v, int depth)
{
    std::stringstream ret;

    switch (v.getType())
    {
        case BValue::Type::NONE:
        case BValue::Type::BYTE:
        case BValue::Type::INTEGER:
        case BValue::Type::FLOAT:
        case BValue::Type::DOUBLE:
        case BValue::Type::BOOLEAN:
        case BValue::Type::LONG_DOUBLE:
        case BValue::Type::STRING:
		case BValue::Type::VEC2:
		case BValue::Type::SIZE:
		case BValue::Type::COLOR3B:
		case BValue::Type::COLOR4F:
            ret << v.getStringForVisit() << "\n";
            break;
        case BValue::Type::VECTOR:
            ret << visitVector(v.getValueVector(), depth);
            break;
        case BValue::Type::MAP:
            ret << visitMap(v.getValueMap(), depth);
            break;
        case BValue::Type::UN_MAP:
            ret << visitMap(v.getValueUnMap(), depth);
        break;
        case BValue::Type::INT_MAP:
            ret << visitMap(v.getValueIntMap(), depth);
        break;
        case BValue::Type::INT_UN_MAP:
            ret << visitMap(v.getValueIntUnMap(), depth);
        break;
       
        default:
            CCASSERT(false, "Invalid type!");
            break;
    }

    return ret.str();
}
bool BValue::isSimpleType() const
{
    bool result = false;
    
    switch ( getType() )
    {
        case BValue::Type::NONE:
        case BValue::Type::BYTE:
        case BValue::Type::INTEGER:
        case BValue::Type::FLOAT:
        case BValue::Type::DOUBLE:
        case BValue::Type::BOOLEAN:
        case BValue::Type::STRING:
        case BValue::Type::LONG_DOUBLE:
        case BValue::Type::VEC2:
        case BValue::Type::SIZE:
        case BValue::Type::COLOR3B:
        case BValue::Type::COLOR4F:
            result = true;
        break;
        
        case BValue::Type::VECTOR:
        case BValue::Type::MAP:
        case BValue::Type::UN_MAP:
        case BValue::Type::INT_MAP:
        case BValue::Type::INT_UN_MAP:
            result = false;
        break;
        
        default:
            CCASSERT(false, "Invalid type!");
        break;
    }
    
    return result;
}
bool BValue::checkType( Type type ) const
{
    bool ret = false;
    
    if ( type == _type )
    {
        ret = true;
    }
    
    return ret;
}
std::string BValue::getDescription() const
{
    std::string ret("\n");
    ret += visit(*this, 0);
    return ret;
}
void BValue::clear()
{
    // Free memory the old value allocated
    switch (_type)
    {
        case Type::BYTE:
            _field.byteVal = 0;
            break;
        case Type::INTEGER:
            _field.intVal = 0;
            break;
        case Type::FLOAT:
            _field.floatVal = 0.0f;
            break;
        case Type::DOUBLE:
            _field.doubleVal = 0.0;
            break;
        case Type::BOOLEAN:
            _field.boolVal = false;
            break;
        case Type::STRING:
            CC_SAFE_DELETE(_field.strVal);
            break;
        case Type::LONG_DOUBLE:
            CC_SAFE_DELETE(_field.longDoubleVal);
            break;
        case Type::VEC2:
            CC_SAFE_DELETE(_field.vec2Val);
			break;
		case Type::SIZE:
			CC_SAFE_DELETE(_field.sizeVal);
			break;
		case Type::COLOR3B:
			CC_SAFE_DELETE(_field.color3BVal);
			break;
		case Type::COLOR4F:
			CC_SAFE_DELETE(_field.color4FVal);
			break;
        case Type::VECTOR:
            CC_SAFE_DELETE(_field.vectorVal);
            break;
        case Type::MAP:
            CC_SAFE_DELETE(_field.mapVal);
            break;
        case Type::UN_MAP:
            CC_SAFE_DELETE(_field.unMapVal);
            break;
        case Type::INT_MAP:
            CC_SAFE_DELETE(_field.intMapVal);
        break;
        case Type::INT_UN_MAP:
            CC_SAFE_DELETE(_field.intUnMapVal);
        break;
        default:
            break;
    }
    
    _type = Type::NONE;
}

void BValue::reset(Type type, bool force )
{
    if ( !force && ( _type == type ) )
        return;

    clear();

    // Allocate memory for the new value
    switch (type)
    {
        case Type::STRING:
            _field.strVal = new std::string();
            break;
        case Type::LONG_DOUBLE:
            _field.longDoubleVal = new long double;
            break;
        case Type::VEC2:
            _field.vec2Val = new Vec2();
            break;
		case Type::SIZE:
			_field.sizeVal = new Size();
			break;
		case Type::COLOR3B:
			_field.color3BVal = new Color3B();
			break;
		case Type::COLOR4F:
			_field.color4FVal = new Color4F();
			break;
        case Type::VECTOR:
            _field.vectorVal = new (std::nothrow) BValueVector();
            break;
        case Type::MAP:
            _field.mapVal = new (std::nothrow) BValueMap();
            break;
        case Type::UN_MAP:
            _field.unMapVal = new (std::nothrow) BValueUnMap();
        break;
        case Type::INT_MAP:
            _field.intMapVal = new (std::nothrow) BValueIntMap();
        break;
        case Type::INT_UN_MAP:
            _field.intUnMapVal = new (std::nothrow) BValueIntUnMap();
            break;
        default:
            break;
    }

    _type = type;
}
bool BValue::initWithFormatAndValist(const char* format, va_list ap)
{
    bool bRet = false;
    
    if ( _type == Type::STRING )
    {
        char* pBuf = (char*)malloc(kMaxStringLen);
        if (pBuf != nullptr)
        {
            vsnprintf(pBuf, kMaxStringLen, format, ap);
            *_field.strVal = pBuf;
        free(pBuf);
        bRet = true;
    }
    }
    return bRet;
}

bool BValue::initWithFormat(const char* format, ...)
{
    bool bRet = false;
    
    reset(Type::STRING);
   
    _field.strVal->clear();
    
    va_list ap;
    va_start(ap, format);
    
    bRet = initWithFormatAndValist(format, ap);
    
    va_end(ap);
    
    return bRet;
}
void BValue::clearWithType( Type type )
{
    reset( type, true );
}
