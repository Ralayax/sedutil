/* C:B**************************************************************************
This software is Copyright 2014-2017 Bright Plaza Inc. <drivetrust@drivetrust.com>

This file is part of sedutil.

sedutil is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

sedutil is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with sedutil.  If not, see <http://www.gnu.org/licenses/>.

 * C:E********************************************************************** */
#include "os.h"
#include <iostream>
#include <iomanip>
#include <string_view>
#include <charconv>
#include "DtaHashPwd.h"
#include "DtaLexicon.h"
#include "DtaOptions.h"
#include "DtaDev.h"
#include "log.h"

extern "C" {
#include "pbkdf2.h"
#include "sha1.h"
#include "sha2.h"
#include "sha3.h"
}
using namespace std;

/** Converts the hashing function enum to the relevant function pointer.
 * Used to not expose the interna cifra library
 * @param primitive The hashing function the user wants a pointer to
 */
const cf_chash* hashFnPtr(HashingAlgorithm::Function const& primitive) {
    switch(primitive) {
        case HashingAlgorithm::Function::sha1 :
            return &cf_sha1;
        case HashingAlgorithm::Function::sha2_256 :
            return &cf_sha256;
        case HashingAlgorithm::Function::sha2_384 :
            return &cf_sha384;
        case HashingAlgorithm::Function::sha2_512 :
            return &cf_sha512;
        case HashingAlgorithm::Function::sha3_256 :
            return &cf_sha3_256;
        case HashingAlgorithm::Function::sha3_384 :
            return &cf_sha3_384;
        case HashingAlgorithm::Function::sha3_512 :
            return &cf_sha3_512;
        default :
            return &cf_sha3_512;
    }
}

vector<uint8_t> decodeHexPassword(std::string_view password) {
    vector<uint8_t> decoded_password;
    decoded_password.reserve(password.length()/2);
    for (unsigned int i = 0; i+1 < password.length(); i += 2) {
        std::string_view pairOfChars = password.substr(i, 2);
        int byte = 0;
        if (std::from_chars(pairOfChars.begin(), pairOfChars.end(), byte, 16).ec != std::errc{}) {
            LOG(E) << "Invalid hex characters provided";
            //abort();
        }
        LOG(E) << pairOfChars;
        decoded_password.push_back(byte);
    }
    return decoded_password;
}

vector<uint8_t> decodePassword(string_view password, bool hex_passwords) {
    if(!hex_passwords) {
        return vector<uint8_t>(password.begin(), password.end());
    }
    
    vector<uint8_t> decodedPassword = decodeHexPassword(password);
    return decodedPassword;
}

std::vector<uint8_t> serialNumToSalt(char* serialNum) {
    return std::vector<uint8_t>(serialNum, serialNum+20);
}

std::vector<uint8_t> HashingAlgorithm::hash(vector<uint8_t> const& password) const {
    LOG(D1) << " Entered HashingAlgorithm::hash";

	// if the hashsize can be > 255 the token overhead logic needs to be fixed
	assert(1 == sizeof(hashsize));
	if (253 < hashsize) {
        LOG(E) << "Hashsize > 253 incorrect token generated";
        //abort();
    }
	
	// don't hash the default OPAL password '' and just return the token header
	if (0 == password.size()) {
		return {0xd0, 0x00};
	}
    
    vector<uint8_t> hash(hashsize + 2);
    hash[0] = 0xd0;
    hash[1] = (uint8_t)hashsize;

    auto true_hash_begin = hash.begin()+2;
	
	cf_pbkdf2_hmac(password.data(), password.size(),
		salt.data(), salt.size(),
		iter,
		&*true_hash_begin, hashsize,
		hashFnPtr(function));

    return hash;
}

std::vector<uint8_t> NoHashing::hash(vector<uint8_t> const& password) const {
    int hashsize = std::min(password.size(), 32UL);

    std::vector<uint8_t> hash;
    hash.reserve(hashsize + 2);
    hash[0] = 0xd0;
    hash[1] = (uint8_t)hashsize;
    hash.insert(hash.end(), password.begin(), password.end());

    return hash;
}

std::vector<uint8_t> PasswordProcessor::process(std::string_view password) const {
    vector<uint8_t> decodedPassword;
    if(!hex_password) {
        decodedPassword = vector<uint8_t>(password.begin(), password.end());
    } else {
        decodedPassword = decodeHexPassword(password);
    }

    if(auto const* hashingAlgorithm = std::get_if<HashingAlgorithm>(&hasher)) {
        return hashingAlgorithm->hash(decodedPassword);
    } else {
        auto const* noHashing = std::get_if<NoHashing>(&hasher);
        return noHashing->hash(decodedPassword);
    }
}

void DtaHashPwd(vector<uint8_t> &hash, char * password, DtaDev * d)
{
    switch (d->password_hashing_options) {
        case no_hashing: {
            PasswordProcessor processor {d->hex_passwords,
                                     NoHashing()};
            hash = processor.process(password);
            break;
        }
        case dta_preset:{
            PasswordProcessor processor {d->hex_passwords,
                                     HashingAlgorithm::dtaPreset(serialNumToSalt(d->getSerialNum()))};
            hash = processor.process(password);
            break;
        }
        case ladar_preset:{
            PasswordProcessor processor {d->hex_passwords,
                                     HashingAlgorithm::ladarPreset(serialNumToSalt(d->getSerialNum()))};
            hash = processor.process(password);
            break;
        }
        case chubbyant_preset:{
            PasswordProcessor processor {d->hex_passwords,
                                     HashingAlgorithm::chubbyAntPreset(serialNumToSalt(d->getSerialNum()))};
            hash = processor.process(password);
            break;
        }
        case ralayax_preset:{
            PasswordProcessor processor {d->hex_passwords,
                                     HashingAlgorithm::ralayaxPreset(serialNumToSalt(d->getSerialNum()))};
            hash = processor.process(password);
            break;
        } 
    }

    LOG(D1) << " Exit DtaHashPwd"; // log for hash timing
}


struct PBKDF_TestTuple
{
    uint8_t hashlen;
    unsigned int iterations;
    const char *Password, *Salt, *hexDerivedKey;
};

int testresult(std::vector<uint8_t> &result, const char * expected, size_t len) {
	char work[50];
	if (len > 50) return 1;
	int p = 0;
	printf("Expected Result: %s\nActual Result  : ", expected);
	for (uint32_t i = 0; i < len; i++) { printf("%02x", result[i + 2]); }; printf("\n");
	for (uint32_t i = 0; i < len * 2; i += 2) {
		work[p] = expected[i] & 0x40 ? 16 * ((expected[i] & 0xf) + 9) : 16 * (expected[i] & 0xf);
		work[p] += expected[i + 1] & 0x40 ? (expected[i + 1] & 0xf) + 9 : expected[i + 1] & 0xf;
		p++;
	}
	return memcmp(result.data()+2, work, len);
}

int Testsedutil(const PBKDF_TestTuple *testSet, unsigned int testSetSize)
{   
    int pass = 1;
    std::vector<uint8_t> hash, seaSalt, password;

    for (unsigned int i = 0; i < testSetSize; i++) {
        const PBKDF_TestTuple &tuple = testSet[i];
        hash.clear();
        seaSalt.clear();
        for (uint16_t j = 0; j < strnlen(tuple.Salt, 255); j++) {
            seaSalt.push_back(tuple.Salt[j]);
        }
		printf("Password %s Salt %s Iterations %i Length %i\n", (char *)tuple.Password,
			(char *) tuple.Salt, tuple.iterations, tuple.hashlen);
        password.assign(tuple.Password, tuple.Password+strlen(tuple.Password));
        HashingAlgorithm algorithm{HashingAlgorithm::Function::sha1, tuple.iterations, tuple.hashlen, seaSalt};
        hash = algorithm.hash(password);
		int fail = (testresult(hash, tuple.hexDerivedKey, tuple.hashlen) == 0);
        pass = pass & fail;
    }

    return pass;
}
int TestPBKDF2()
{
    int pass = 1;
    // from draft-ietf-smime-password-03.txt, at http://www.imc.org/draft-ietf-smime-password
    PBKDF_TestTuple testSet[] = {
        // Draft PKCS #5 PBKDF2 Test Vectors http://tools.ietf.org/html/draft-josefsson-pbkdf2-test-vectors-06
        // "password" (8 octets) S = "salt" (4 octets)	c = 1 DK = 0c60c80f961f0e71f3a9b524af6012062fe037a6
        { 20, 1, "password", "salt", "0c60c80f961f0e71f3a9b524af6012062fe037a6"},
        // "password" (8 octets) S = "salt" (4 octets)	c = 2 DK = ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957
        { 20, 2, "password", "salt", "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957"},
        // "password" (8 octets) S = "salt" (4 octets)	c = 4096 DK = 4b007901b765489abead49d926f721d065a429c1
        { 20, 4096, "password", "salt", "4b007901b765489abead49d926f721d065a429c1"},
        // "password" (8 octets) S = "salt" (4 octets)	c = 16777216 DK = eefe3d61cd4da4e4e9945b3d6ba2158c2634e984
        //{ 20, 16777216, "password", "salt", "eefe3d61cd4da4e4e9945b3d6ba2158c2634e984" },
        // "passwordPASSWORDpassword" (24 octets) S = "saltSALTsaltSALTsaltSALTsaltSALTsalt" (36 octets) c = 4096 DK = 3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038
        { 25, 4096, "passwordPASSWORDpassword", "saltSALTsaltSALTsaltSALTsaltSALTsalt",
            "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038"},
        // "pass\0word" (9 octets) S = "sa\0lt" (5 octets)	c = 4096 DK = 56fa6aa75548099dcc37d7f03425e0c3
        //{ 16, 4096, "pass\0word", "sa\0lt", "56fa6aa75548099dcc37d7f03425e0c3" },
        // program receives char * from OS so this test would fail but is not possible IRL
    };

    cout << "\nPKCS #5 PBKDF2 validation suite running ... \n\n";
    pass = Testsedutil(testSet, sizeof (testSet) / sizeof (testSet[0])) && pass;
    cout << "\nPKCS #5 PBKDF2 validation suite ... ";
    if (pass)
        cout << "passed\n";
    else
        cout << "**FAILED**\n";
    return 0;
}

