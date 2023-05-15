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
#include <variant>
#include <cstdint>
#include <string_view>
class DtaDev;

/** Dummy class describing the hashing algorithm to be used for hashing the password
 */
struct NoHashing {
        std::vector<uint8_t> hash(std::vector<uint8_t> const& password) const;
};

/** Class describing the hashing algorithm to be used for hashing the password
 * A few presets are provided for compatibility
 * with the different most used forks of sedutil.
 *
 * @param hash Field where hash returned
 * @param password password to be hashed
 * @param salt salt to be used in the hash
 * @param iter number of iterations to be preformed 
 * @param hashsize size of hash to be returned
 */
struct HashingAlgorithm {
        enum class Function {
                sha1,
                sha2_256,
                sha2_384,
                sha2_512,
                sha3_256,
                sha3_384,
                sha3_512,
        } function;
        unsigned int iter; /**< Iteration number */
        uint8_t hashsize; /**< Hash output size */
        std::vector<uint8_t> salt; /**< Salt */

        /** DTA Preset constructor, backwards compatible with official DTA sedutil
         * Uses 75000 of unsecure sha1
         *
         * @param salt The hashing salt to be used
         */
        static HashingAlgorithm dtaPreset(std::vector<uint8_t> const& salt) {
                return {
                        Function::sha1,
                        75000,
                        32,
                        salt
                };
        }

        /** Ladar Preset constructor, compatible with ladar sedutil
         * Uses 75000 iterations of sha2_512
         *
         * @param salt The hashing salt to be used
         */
        static HashingAlgorithm ladarPreset(std::vector<uint8_t> const& salt) {
                return {
                        Function::sha2_512,
                        75000,
                        32,
                        salt
                };
        }

        /** ChubbyAnt Preset constructor, compatible with ChubbyAnt sedutil
         * Uses 500000 iterations of sha2_512
         *
         * @param salt The hashing salt to be used
         */
        static HashingAlgorithm chubbyAntPreset(std::vector<uint8_t> const& salt) {
                return {
                        Function::sha2_512,
                        500000,
                        32,
                        salt
                };
        }

        /** Ralayax Preset constructor, compatible with Ralayax sedutil
         * Uses 500000 iterations of sha3_512
         *
         * @param salt The hashing salt to be used
         */
        static HashingAlgorithm ralayaxPreset(std::vector<uint8_t> const& salt) {
                return {
                        Function::sha3_512,
                        500000,
                        32,
                        salt
                };
        }

        std::vector<uint8_t> hash(std::vector<uint8_t> const& password) const;
};

struct PasswordProcessor {
        bool hex_password;
        std::variant<NoHashing, HashingAlgorithm> hasher;

        std::vector<uint8_t> process(std::string_view password) const;
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
void DtaHashPwd(std::vector<uint8_t> &hash, char * password, DtaDev * device);

/** Test the hshing function using publicly available test cased and report */
int TestPBKDF2();
