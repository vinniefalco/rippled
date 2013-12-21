//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

namespace ripple {

STParsedJSON::STParsedJSON (std::string const& name, Json::Value const& json)
{
    parse (name, json, sfGeneric, 0, object);
}

//------------------------------------------------------------------------------

std::string STParsedJSON::make_name (std::string const& object,
    std::string const& field)
{
    if (field.empty ())
        return object;

    return object + "." + field;
}

Json::Value STParsedJSON::not_an_object (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' is not a JSON object.");
}

Json::Value STParsedJSON::unknown_field (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' is unknown.");
}

Json::Value STParsedJSON::out_of_range (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' is out of range.");
}

Json::Value STParsedJSON::bad_type (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' has bad type.");
}

Json::Value STParsedJSON::invalid_data (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' has invalid data.");
}

Json::Value STParsedJSON::array_expected (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' must be a JSON array.");
}

Json::Value STParsedJSON::string_expected (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' must be a string.");
}

Json::Value STParsedJSON::too_deep (std::string const& object,
    std::string const& field)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + make_name (object, field) + "' exceeds nesting depth limit.");
}

Json::Value STParsedJSON::singleton_expected (std::string const& object)
{
    return RPC::make_error (rpcINVALID_PARAMS,
        "Field '" + object +
            "' must be an object with a single key/object value.");
}

//------------------------------------------------------------------------------

bool STParsedJSON::parse (std::string const& json_name,
    Json::Value const& json, SField::ref inName, int depth,
        unique_ptr <STObject>& sub_object)
{
    if (! json.isObject ())
    {
        error = not_an_object (json_name);
        return false;
    }

    SField::ptr name (&inName);

    boost::ptr_vector<SerializedType> data;
    Json::Value::Members members (json.getMemberNames ());

    for (Json::Value::Members::iterator it (members.begin ());
        it != members.end (); ++it)
    {
        std::string const& fieldName = *it;
        Json::Value const& value = json [fieldName];

        SField::ref field = SField::getField (fieldName);

        if (field == sfInvalid)
        {
            error = unknown_field (json_name, fieldName);
            return false;
        }

        switch (field.fieldType)
        {
        case STI_UINT8:
            try
            {
                if (value.isString ())
                {
                    // VFALCO TODO wtf?
                }
                else if (value.isInt ())
                {
                    if (value.asInt () < 0 || value.asInt () > 255)
                    {
                        error = out_of_range (json_name, fieldName);
                        return false;
                    }

                    data.push_back (new STUInt8 (field,
                        range_check_cast <unsigned char> (
                            value.asInt (), 0, 255)));
                }
                else if (value.isUInt ())
                {
                    if (value.asUInt () > 255)
                    {
                        error = out_of_range (json_name, fieldName);
                        return false;
                    }

                    data.push_back (new STUInt8 (field,
                        range_check_cast <unsigned char> (
                            value.asUInt (), 0, 255)));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_UINT16:
            try
            {
                if (value.isString ())
                {
                    std::string strValue = value.asString ();

                    if (! strValue.empty () &&
                        ((strValue[0] < '0') || (strValue[0] > '9')))
                    {
                        if (field == sfTransactionType)
                        {
                            TxType const txType (TxFormats::getInstance()->
                                findTypeByName (strValue));

                            data.push_back (new STUInt16 (field,
                                static_cast <uint16> (txType)));

                            if (*name == sfGeneric)
                                name = &sfTransaction;
                        }
                        else if (field == sfLedgerEntryType)
                        {
                            LedgerEntryType const type (LedgerFormats::getInstance()->
                                findTypeByName (strValue));

                            data.push_back (new STUInt16 (field,
                                static_cast <uint16> (type)));

                            if (*name == sfGeneric)
                                name = &sfLedgerEntry;
                        }
                        else
                        {
                            error = invalid_data (json_name, fieldName);
                            return false;
                        }
                    }
                    else
                    {
                        data.push_back (new STUInt16 (field,
                            lexicalCastThrow <uint16> (strValue)));
                    }
                }
                else if (value.isInt ())
                {
                    data.push_back (new STUInt16 (field,
                        range_check_cast <uint16> (
                            value.asInt (), 0, 65535)));
                }
                else if (value.isUInt ())
                {
                    data.push_back (new STUInt16 (field,
                        range_check_cast <uint16> (
                            value.asUInt (), 0, 65535)));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_UINT32:
            try
            {
                if (value.isString ())
                {
                    data.push_back (new STUInt32 (field,
                        lexicalCastThrow <uint32> (value.asString ())));
                }
                else if (value.isInt ())
                {
                    data.push_back (new STUInt32 (field,
                        range_check_cast <uint32> (value.asInt (), 0u, 4294967295u)));
                }
                else if (value.isUInt ())
                {
                    data.push_back (new STUInt32 (field,
                        static_cast <uint32> (value.asUInt ())));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_UINT64:
            try
            {
                if (value.isString ())
                {
                    data.push_back (new STUInt64 (field,
                        uintFromHex (value.asString ())));
                }
                else if (value.isInt ())
                {
                    data.push_back (new STUInt64 (field,
                        range_check_cast<uint64> (
                            value.asInt (), 0, 18446744073709551615ull)));
                }
                else if (value.isUInt ())
                {
                    data.push_back (new STUInt64 (field,
                        static_cast <uint64> (value.asUInt ())));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_HASH128:
            try
            {
                if (value.isString ())
                {
                    data.push_back (new STHash128 (field, value.asString ()));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_HASH160:
            try
            {
                if (value.isString ())
                {
                    data.push_back (new STHash160 (field, value.asString ()));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_HASH256:
            try
            {
                if (value.isString ())
                {
                    data.push_back (new STHash256 (field, value.asString ()));
                }
                else
                {
                    error = bad_type (json_name, fieldName);
                    return false;
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_VL:
            if (! value.isString ())
            {
                error = bad_type (json_name, fieldName);
                return false;
            }

            try
            {
                data.push_back (new STVariableLength (field,
                    strUnHex (value.asString ())));
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_AMOUNT:
            try
            {
                data.push_back (new STAmount (field, value));
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_VECTOR256:
            if (! value.isArray ())
            {
                error = array_expected (json_name, fieldName);
                return false;
            }

            try
            {
                data.push_back (new STVector256 (field));
                STVector256* tail (dynamic_cast <STVector256*> (&data.back ()));
                check_precondition (tail);

                for (Json::UInt i = 0; !json.isValidIndex (i); ++i)
                {
                    uint256 s;
                    s.SetHex (json[i].asString ());
                    tail->addValue (s);
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_PATHSET:
            if (!value.isArray ())
            {
                error = array_expected (json_name, fieldName);
                return false;
            }

            try
            {
                data.push_back (new STPathSet (field));
                STPathSet* tail = dynamic_cast <STPathSet*> (&data.back ());
                check_precondition (tail);

                for (Json::UInt i = 0; value.isValidIndex (i); ++i)
                {
                    STPath p;

                    if (!value[i].isArray ())
                    {
                        std::stringstream ss;
                        ss << fieldName << "[" << i << "]";
                        error = array_expected (json_name, ss.str ());
                        return false;
                    }

                    for (Json::UInt j = 0; value[i].isValidIndex (j); ++j)
                    {
                        std::stringstream ss;
                        ss << fieldName << "[" << i << "][" << j << "]";
                        std::string const element_name (
                            json_name + "." + ss.str());

                        // each element in this path has some combination of account,
                        // currency, or issuer

                        Json::Value pathEl = value[i][j];

                        if (!pathEl.isObject ())
                        {
                            error = not_an_object (element_name);
                            return false;
                        }

                        const Json::Value& account  = pathEl["account"];
                        const Json::Value& currency = pathEl["currency"];
                        const Json::Value& issuer   = pathEl["issuer"];
                        bool hasCurrency            = false;
                        uint160 uAccount, uCurrency, uIssuer;

                        if (! account.isNull ())
                        {
                            // human account id
                            if (! account.isString ())
                            {
                                error = string_expected (element_name, "account");
                                return false;
                            }

                            std::string const strValue (account.asString ());

                            if (value.size () == 40) // 160-bit hex account value
                                uAccount.SetHex (strValue);

                            {
                                RippleAddress a;

                                if (! a.setAccountID (strValue))
                                {
                                    error = invalid_data (element_name, "account");
                                    return false;
                                }

                                uAccount = a.getAccountID ();
                            }
                        }

                        if (!currency.isNull ())
                        {
                            // human currency
                            if (!currency.isString ())
                            {
                                error = string_expected (element_name, "currency");
                                return false;
                            }

                            hasCurrency = true;

                            if (currency.asString ().size () == 40)
                            {
                                uCurrency.SetHex (currency.asString ());
                            }
                            else if (!STAmount::currencyFromString (
                                uCurrency, currency.asString ()))
                            {
                                error = invalid_data (element_name, "currency");
                                return false;
                            }
                        }

                        if (!issuer.isNull ())
                        {
                            // human account id
                            if (!issuer.isString ())
                            {
                                error = string_expected (element_name, "issuer");
                                return false;
                            }

                            if (issuer.asString ().size () == 40)
                            {
                                uIssuer.SetHex (issuer.asString ());
                            }
                            else
                            {
                                RippleAddress a;

                                if (!a.setAccountID (issuer.asString ()))
                                {
                                    error = invalid_data (element_name, "issuer");
                                    return false;
                                }

                                uIssuer = a.getAccountID ();
                            }
                        }

                        p.addElement (STPathElement (uAccount, uCurrency, uIssuer, hasCurrency));
                    }

                    tail->addPath (p);
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_ACCOUNT:
        {
            if (! value.isString ())
            {
                error = bad_type (json_name, fieldName);
                return false;
            }

            std::string strValue = value.asString ();

            try
            {
                if (value.size () == 40) // 160-bit hex account value
                {
                    uint160 v;
                    v.SetHex (strValue);
                    data.push_back (new STAccount (field, v));
                }
                else
                {
                    // ripple address
                    RippleAddress a;

                    if (!a.setAccountID (strValue))
                    {
                        error = invalid_data (json_name, fieldName);
                        return false;
                    }

                    data.push_back (new STAccount (field, a.getAccountID ()));
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }
        }
        break;

        case STI_OBJECT:
        case STI_TRANSACTION:
        case STI_LEDGERENTRY:
        case STI_VALIDATION:
            if (! value.isObject ())
            {
                error = not_an_object (json_name, fieldName);
                return false;
            }

            if (depth > 64)
            {
                error = too_deep (json_name, fieldName);
                return false;
            }

            try
            {
                unique_ptr <STObject> sub_object_;
                bool const success (parse (json_name + "." + fieldName,
                    value, field, depth + 1, sub_object_));
                if (! success)
                    return false;
                data.push_back (sub_object_.release ());
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        case STI_ARRAY:
            if (! value.isArray ())
            {
                error = array_expected (json_name, fieldName);
                return false;
            }

            try
            {
                data.push_back (new STArray (field));
                STArray* tail = dynamic_cast<STArray*> (&data.back ());
                check_precondition (tail);

                for (Json::UInt i = 0; value.isValidIndex (i); ++i)
                {
                    bool const isObject (value[i].isObject());
                    bool const singleKey (isObject
                        ? value [i].size() == 1
                        : true);

                    if (!isObject || !singleKey)
                    {
                        std::stringstream ss;
                        ss << json_name << "." << fieldName << "[" << i << "]";
                        error = singleton_expected (ss.str ());
                        return false;
                    }

                    // TODO: There doesn't seem to be a nice way to get just the
                    // first/only key in an object without copying all keys into
                    // a vector
                    std::string const objectName (value[i].getMemberNames()[0]);;
                    SField::ref const nameField (SField::getField(objectName));
                    Json::Value const objectFields (value[i][objectName]);

                    unique_ptr <STObject> sub_object_;
                    {
                        std::stringstream ss;
                        ss << json_name << "." << fieldName <<
                            "[" << i << "]." << objectName;
                        bool const success (parse (ss.str (), objectFields,
                            nameField, depth + 1, sub_object_));
                        if (! success)
                            return false;
                    }
                    tail->push_back (*sub_object_.release ());
                }
            }
            catch (...)
            {
                error = invalid_data (json_name, fieldName);
                return false;
            }

            break;

        default:
            error = bad_type (json_name, fieldName);
            return false;
        }
    }

    sub_object = new STObject (*name, data);
    return true;
}

//------------------------------------------------------------------------------

class STParsedJSONTests : public UnitTest
{
public:
    STParsedJSONTests () : UnitTest ("STParsedJSON", "ripple")
    {
    }

    bool parseJSONString (const std::string& json, Json::Value& to)
    {
        Json::Reader reader;
        return (reader.parse(json, to) &&
               !to.isNull() &&
                to.isObject());
    }

    std::string serializeJsonValue (const Json::Value& json)
    {
        Json::FastWriter writer;
        return writer.write(json);
    }

    void jsonParsingTestCase (std::string const& testName,
                              std::string expectedError,
                              std::string const& text)
    {
        beginTestCase (testName);
        // Json::FastWriter always appends a `\n`, this is the easiest way to
        // normalize the two values for later comparison;
        expectedError.append("\n");

        Json::Value jsonObject;
        bool parsedOK (parseJSONString(text, jsonObject));
        if (parsedOK)
        {
            STParsedJSON parsed ("tx_json", jsonObject);
            std::string const actualError (serializeJsonValue(parsed.error));
            unexpected ( actualError != expectedError,
                         "Expected error: " + expectedError +
                         "Got error:      " + actualError );
        }
        else
        {
            fail ("Error: couldn't parse Json::Value from: " + text);
        }
    }

    void jsonParsingTestCase (std::string const& testName,
                                              std::string const& text)
    {
        jsonParsingTestCase(testName, "null", text);
    }

    void testTransactionWithTransactionMetaData ()
    {
        jsonParsingTestCase (
            "transaction with metadata",
            // "What the fffffff",
            "{"
            "  \"Account\": \"rMWUykAmNQDaM9poSes8VLDZDDKEbmo7MX\","
            "  \"Fee\": \"10\","
            "  \"Flags\": 0,"
            "  \"OfferSequence\": 701424,"
            "  \"Sequence\": 701542,"
            "  \"SigningPubKey\": \"0256C64F0378DCCCB4E0224B36F7ED1E5586455FF105F760245ADB35A8B03A25FD\","
            "  \"TransactionType\": \"OfferCancel\","
            "  \"TxnSignature\": \"3046022100E6F4B1830613531489EBBD8E97B086DA3893A8518904A40BB0567A40ABB6C71A022100F1BE31EA455441F609207D90EA7F42D9F45412AF01541049489EE6487D51851D\","
            "  \"hash\": \"2D959E98A6A914BFA789751A7A31536D893B1A12D69A833FA56207C067E6C637\","
            "  \"TransactionMetaData\": {"
            "    \"AffectedNodes\": ["
            "      {"
            "        \"ModifiedNode\": {"
            "          \"FinalFields\": {"
            "            \"Flags\": 0,"
            "            \"IndexNext\": \"0000000000001EA1\","
            "            \"IndexPrevious\": \"0000000000001E9F\","
            "            \"Owner\": \"rMWUykAmNQDaM9poSes8VLDZDDKEbmo7MX\","
            "            \"RootIndex\": \"2114A41BB356843CE99B2858892C8F1FEF634B09F09AF2EB3E8C9AA7FD0E3A1A\""
            "          },"
            "          \"LedgerEntryType\": \"DirectoryNode\","
            "          \"LedgerIndex\": \"508544FFDC3D3059BEBF150C3AEA1D04AD1F69FA0321C8C2C583946E123BC42F\""
            "        }"
            "      },"
            "      {"
            "        \"ModifiedNode\": {"
            "          \"FinalFields\": {"
            "            \"Account\": \"rMWUykAmNQDaM9poSes8VLDZDDKEbmo7MX\","
            "            \"Balance\": \"1992984067\","
            "            \"Flags\": 0,"
            "            \"OwnerCount\": 55,"
            "            \"Sequence\": 701543"
            "          },"
            "          \"LedgerEntryType\": \"AccountRoot\","
            "          \"LedgerIndex\": \"56091AD066271ED03B106812AD376D48F126803665E3ECBFDBBB7A3FFEB474B2\","
            "          \"PreviousFields\": {"
            "            \"Balance\": \"1992984077\","
            "            \"OwnerCount\": 56,"
            "            \"Sequence\": 701542"
            "          },"
            "          \"PreviousTxnID\": \"3CFB86480839AD620B8A6217BE1832DD886EB42013829FECB34DE235D43FE87E\","
            "          \"PreviousTxnLgrSeq\": 2596381"
            "        }"
            "      },"
            "      {"
            "        \"ModifiedNode\": {"
            "          \"FinalFields\": {"
            "            \"ExchangeRate\": \"5616323607D3C710\","
            "            \"Flags\": 0,"
            "            \"RootIndex\": \"5943CB2C05B28743AADF0AE47E9C57E9C15BD23284CF6DA95616323607D3C710\","
            "            \"TakerGetsCurrency\": \"0000000000000000000000004254430000000000\","
            "            \"TakerGetsIssuer\": \"92D705968936C419CE614BF264B5EEB1CEA47FF4\","
            "            \"TakerPaysCurrency\": \"0000000000000000000000004C54430000000000\","
            "            \"TakerPaysIssuer\": \"92D705968936C419CE614BF264B5EEB1CEA47FF4\""
            "          },"
            "          \"LedgerEntryType\": \"DirectoryNode\","
            "          \"LedgerIndex\": \"5943CB2C05B28743AADF0AE47E9C57E9C15BD23284CF6DA95616323607D3C710\""
            "        }"
            "      },"
            "      {"
            "        \"DeletedNode\": {"
            "          \"FinalFields\": {"
            "            \"Account\": \"rMWUykAmNQDaM9poSes8VLDZDDKEbmo7MX\","
            "            \"BookDirectory\": \"5943CB2C05B28743AADF0AE47E9C57E9C15BD23284CF6DA95616323607D3C710\","
            "            \"BookNode\": \"0000000000000000\","
            "            \"Flags\": 0,"
            "            \"OwnerNode\": \"0000000000001EA0\","
            "            \"PreviousTxnID\": \"5CF4FF1539346CE8FC2EF120E746D292EFB64B54589434EA6CC0FBFAFF4E8C05\","
            "            \"PreviousTxnLgrSeq\": 2596240,"
            "            \"Sequence\": 701424,"
            "            \"TakerGets\": {"
            "              \"currency\": \"BTC\","
            "              \"issuer\": \"rNPRNzBB92BVpAhhZr4iXDTveCgV5Pofm9\","
            "              \"value\": \"0.4032663682\""
            "            },"
            "            \"TakerPays\": {"
            "              \"currency\": \"LTC\","
            "              \"issuer\": \"rNPRNzBB92BVpAhhZr4iXDTveCgV5Pofm9\","
            "              \"value\": \"25.1947\""
            "            }"
            "          },"
            "          \"LedgerEntryType\": \"Offer\","
            "          \"LedgerIndex\": \"EF2C9F1C59DD91E1545DA9483258D6C90C344A640C1088B1EE054E428FF49F99\""
            "        }"
            "      }"
            "    ],"
            "    \"TransactionIndex\": 0,"
            "    \"TransactionResult\": \"tesSUCCESS\""
            "  }"
            "}"
        );
    }

    void testMalformedAccount()
    {
        jsonParsingTestCase (
            "malformed account",
            ("{\"error\":\"invalidParams\","
              "\"error_code\":25,"
              "\"error_message\":"
                 "\"Field 'tx_json.Account' has invalid data.\"}"),
            "{\"Account\" : \"seriously!!???\"}"
        );
    }

    void testMalformedFieldInArray()
    {
        jsonParsingTestCase (
            "malformed account",
            ("{\"error\":\"invalidParams\","
              "\"error_code\":25,"
              "\"error_message\":"
                 "\"Field 'tx_json.AffectedNodes[0].ModifiedNode.Account' has invalid data.\"}"),
            "{\"AffectedNodes\" : [{\"ModifiedNode\" : {\"Account\" : \"seriously!!???\"}}]}"
        );
    }

    void runTest ()
    {
        testTransactionWithTransactionMetaData();
        testMalformedAccount();
        testMalformedFieldInArray();
    }
};

static STParsedJSONTests stParsedJSONTests;

// </ripple-namespace>
};
