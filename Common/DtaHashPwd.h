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
#pragma once
#include <vector>
#include <stdint.h>
class DtaDev;

using namespace std;

/** Class describing the hashing algorithm to be used for hashing the password
 * A few presets are provided for compatibility
 * with the different most used forks of sedutil.
*/
struct HashAlgorithm {
        enum class Function {
                sha1,
                sha2_256,
                sha2_384,
                sha2_512,
                sha3_256,
                sha3_384,
                sha3_512,
        };

        Function function;
        unsigned int iter;
        uint8_t hashsize;

        static HashAlgorithm dtaPreset() {
                return {
                        Function::sha1,
                        75000,
                        32
                };
        }

        static HashAlgorithm ladarPreset() {
                return {
                        Function::sha2_512,
                        75000,
                        32
                };
        }

        static HashAlgorithm chubbyAntPreset() {
                return {
                        Function::sha2_512,
                        500000,
                        32
                };
        }

        static HashAlgorithm ralayaxPreset() {
                return {
                        Function::sha3_512,
                        500000,
                        32
                };
        }
};

/** Hash the password using the drive serial number as salt.
 * This is far from ideal but it's better that a single salt as
 * it should prevent attacking the password with a prebuilt table
 * 
 * This is an intermediary pass through so that the real hash
 * function (DtaHashPassword) can be tested and verified.
 * @param  hash The field whare the hash is to be placed
 * @param password The password to be hashed
 * @param device the device where the password is to be used
 */
void DtaHashPwd(vector<uint8_t> &hash, char * password, DtaDev * device);

/** Test the hshing function using publicly available test cased and report */
int TestPBKDF2();
