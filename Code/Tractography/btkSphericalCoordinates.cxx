/*
Copyright or © or Copr. Université de Strasbourg - Centre National de la Recherche Scientifique

22 march 2010
< pontabry at unistra dot fr >

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software. You can use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty and the software's author, the holder of the
economic rights, and the successive licensors have only limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading, using, modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean that it is complicated to manipulate, and that also
therefore means that it is reserved for developers and experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and, more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#include "btkSphericalCoordinates.h"


namespace btk
{

SphericalCoordinates::SphericalCoordinates()
{
    m_theta = 0;
    m_phi   = 0;
    m_rho   = 0;
}

SphericalCoordinates::SphericalCoordinates(Real theta, Real phi, Real rho)
{
    m_theta = theta;
    m_phi   = phi;
    m_rho   = rho;
}

Real SphericalCoordinates::theta() const
{
    return m_theta;
}

Real SphericalCoordinates::phi() const
{
    return m_phi;
}

Real SphericalCoordinates::rho() const
{
    return m_rho;
}

CartesianCoordinates SphericalCoordinates::toCartesianCoordinates()
{
    Real cosTheta = std::cos(m_theta);
    Real sinTheta = std::sin(m_theta);
    Real cosPhi   = std::cos(m_phi);
    Real sinPhi   = std::sin(m_phi);

    return CartesianCoordinates(
        m_rho * sinTheta * cosPhi,
        m_rho * sinTheta * sinPhi,
        m_rho * cosTheta);
}

std::ostream &operator<<(std::ostream &os, const SphericalCoordinates& u)
{
    return os << "(" << u.m_theta << ", " << u.m_phi << ", " << u.m_rho << ")";
}

} // namespace btk

