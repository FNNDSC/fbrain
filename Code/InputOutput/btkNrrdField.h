/*
Copyright or © or Copr. Université de Strasbourg - Centre National de la Recherche Scientifique

19 may 2011
< pontabry at unistra dot fr >

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/


#ifndef __BTK_NRRD_FIELD_H__
#define __BTK_NRRD_FIELD_H__

    // STL includes
    #include "string"

namespace btk
{

    /**
     * @class btkNrrdField
     * @ingroup InputOutput
     */
    class btkNrrdField
    {
        public:
            /**
             * @brief Constructor
             * @param field Field in Nrrd format (key:=value)
             */
            btkNrrdField(std::string field);

            /**
             * @brief Destructor
             */
            ~btkNrrdField();

            /**
             * @brief Accessor
             * @return The string corresponding to the key of the field
             */
            std::string GetKey() const;

            /**
             * @brief Accessor
             * @return The string corresponding to the value of the field
             */
            std::string GetValue() const;

        private:
            class btkNrrdFieldPriv;
            btkNrrdFieldPriv* const m;
    };

} // namespace btk

#endif /* __BTK_NRRD_FIELD_H__ */
