#include <iostream>
#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : m_dataset(Dataset(Datatype::UNDEFINED, {})),
          m_isConstant{false},
          m_constantValue{-1}
{
    setUnitSI(1);
}

double
RecordComponent::unitSI() const
{
    return getAttribute("unitSI").get< double >();
}

RecordComponent&
RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    dirty = true;
    return *this;
}

RecordComponent&
RecordComponent::resetDataset(Dataset d)
{
    m_dataset = d;
    dirty = true;
    return *this;
}

Datatype
RecordComponent::getDatatype()
{
    return m_dataset.dtype;
}

uint8_t
RecordComponent::getDimensionality()
{
    return m_dataset.rank;
}

Extent
RecordComponent::getExtent()
{
    return m_dataset.extents;
}

void
RecordComponent::flush(std::string const& name)
{
    if( !written )
    {
        if( m_isConstant )
        {
            Parameter< Operation::CREATE_PATH > path_parameter;
            path_parameter.path = name;
            IOHandler->enqueue(IOTask(this, path_parameter));
            Parameter< Operation::WRITE_ATT > attribute_parameter;
            attribute_parameter.name = "value";
            attribute_parameter.dtype = m_constantValue.dtype;
            attribute_parameter.resource = m_constantValue.getResource();
            IOHandler->enqueue(IOTask(this, attribute_parameter));
            attribute_parameter.name = "shape";
            Attribute a(getExtent());
            attribute_parameter.dtype = a.dtype;
            attribute_parameter.resource = a.getResource();
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        } else
        {
            Parameter< Operation::CREATE_DATASET > ds_parameter;
            ds_parameter.name = name;
            ds_parameter.dtype = getDatatype();
            ds_parameter.extent = getExtent();
            IOHandler->enqueue(IOTask(this, ds_parameter));
        }
        IOHandler->flush();
    }

    while( !m_chunks.empty() )
    {
        IOHandler->enqueue(m_chunks.front());
        m_chunks.pop();
        IOHandler->flush();
    }

    flushAttributes();
}

void
RecordComponent::read()
{
    readBase();
}

void
RecordComponent::readBase()
{
    if( m_isConstant )
    {
        Parameter< Operation::READ_ATT > attribute_parameter;
        attribute_parameter.name = "value";
        IOHandler->enqueue(IOTask(this, attribute_parameter));
        IOHandler->flush();

        using DT = Datatype;
        Attribute a(*attribute_parameter.resource);
        DT dtype = *attribute_parameter.dtype;
        written = false;
        switch( dtype )
        {
            case DT::DOUBLE:
                makeConstant(a.get< double >());
                break;
            case DT::FLOAT:
                makeConstant(a.get< float >());
                break;
            case DT::INT16:
                makeConstant(a.get< int16_t >());
                break;
            case DT::INT32:
                makeConstant(a.get< int32_t >());
                break;
            case DT::INT64:
                makeConstant(a.get< int64_t >());
                break;
            case DT::UINT16:
                makeConstant(a.get< uint16_t >());
                break;
            case DT::UINT32:
                makeConstant(a.get< uint32_t >());
                break;
            case DT::UINT64:
                makeConstant(a.get< uint64_t >());
                break;
            case DT::CHAR:
                makeConstant(a.get< char >());
                break;
            case DT::UCHAR:
                makeConstant(a.get< unsigned char >());
                break;
            case DT::BOOL:
                makeConstant(a.get< bool >());
                break;
            default:
                throw std::runtime_error("Unexpected constant datatype");
        }
        written = true;

        attribute_parameter.name = "shape";
        IOHandler->enqueue(IOTask(this, attribute_parameter));
        IOHandler->flush();
        a = Attribute(*attribute_parameter.resource);
        Extent e;
        switch( *attribute_parameter.dtype )
        {
            case DT::UINT64:
                e.push_back(a.get< uint64_t >());
                break;
            case DT::VEC_UINT64:
                for( auto const& val : a.get< std::vector< uint64_t > >() )
                    e.push_back(val);
            default:
                throw std::runtime_error("Unexpected Attribute datatype for 'shape'");
        }
        resetDataset(Dataset(dtype, e));
    }

    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "unitSI";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::DOUBLE )
        setUnitSI(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'unitSI'");

    readAttributes();
}

MeshRecordComponent::MeshRecordComponent()
        : RecordComponent()
{
    setPosition({0});
}

void
MeshRecordComponent::read()
{
    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "position";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::VEC_FLOAT )
        setPosition(Attribute(*attribute_parameter.resource).get< std::vector< float > >());
    else if( *attribute_parameter.dtype == DT::VEC_DOUBLE )
    {
        std::cerr << "Non-standard attribute datatype for 'position' (should be vector of float, is vector of double)\n";
        std::vector< float > position;
        for( auto const& val : Attribute(*attribute_parameter.resource).get< std::vector< double > >())
            position.push_back(static_cast<float>(val));
        setPosition(position);
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'position'");

    readBase();
}

std::vector< float >
MeshRecordComponent::position() const
{
    return getAttribute("position").get< std::vector< float > >();
}

MeshRecordComponent&
MeshRecordComponent::setPosition(std::vector< float > pos)
{
    setAttribute("position", pos);
    dirty = true;
    return *this;
}
