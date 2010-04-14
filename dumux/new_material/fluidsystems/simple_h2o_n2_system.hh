/*****************************************************************************
 *   Copyright (C) 2009 by Andreas Lauser
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
/*!
 * \file 
 *
 * \brief A fluid system with water and gas as phases and \f$H_2O\f$ and \f$N_2\f$
 *        as components.
 */
#ifndef DUMUX_SIMPLE_H2O_N2_SYSTEM_HH
#define DUMUX_SIMPLE_H2O_N2_SYSTEM_HH

#include <dumux/new_material/idealgas.hh>
#include <dumux/new_material/components/n2.hh>
#include <dumux/new_material/components/h2o.hh>
#include <dumux/new_material/components/simpleh2o.hh>
#include <dumux/new_material/components/tabulatedcomponent.hh>

#include <dumux/new_material/binarycoefficients/h2o_n2.hh>

namespace Dumux
{

/*!
 * \brief A compositional fluid with water and molecular nitrogen as
 *        components in both, the liquid and the gas phase.
 */
template <class TypeTag>
class Simple_H2O_N2_System
{
    typedef Simple_H2O_N2_System<TypeTag> ThisType;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;

    typedef Dumux::IdealGas<Scalar> IdealGas;

public:
    typedef Dumux::SimpleH2O<Scalar> H2O;
    typedef Dumux::N2<Scalar> N2;

    static const int numComponents = 2;
    static const int numPhases = 2;

    static const int lPhaseIdx = 0; // index of the liquid phase 
    static const int gPhaseIdx = 1; // index of the gas phase 

    static const int wPhaseIdx = lPhaseIdx; // index of the wetting phase 
    static const int nPhaseIdx = gPhaseIdx; // index of the non-wetting phase 

    static const int H2OIdx = 0;
    static const int N2Idx = 1;
    
    static void init()
    { }

    /*!
     * \brief Return the human readable name of a component
     */
    static const char *componentName(int compIdx)
    {
        switch (compIdx) {
        case H2OIdx: return H2O::name();
        case N2Idx: return N2::name();
        };
        DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
    }

    /*!
     * \brief Return the molar mass of a component in [kg/mol].
     */
    static Scalar molarMass(int compIdx)
    {
        switch (compIdx) {
        case H2OIdx: return H2O::molarMass();
        case N2Idx: return N2::molarMass();
        };
        DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
    }
    
    /*!
     * \brief Given the gas phase's composition, temperature and
     *        pressure, compute the partial presures of all components
     *        in [Pa] and assign it to the FluidState.
     *
     * This is required for models which cannot calculate the the
     * partial pressures of the components in the gas phase from the
     * degasPressure(). To use this method, the FluidState must have a
     * setPartialPressure(componentIndex, pressure) method.
     */
    template <class FluidState>
    static void computePartialPressures(Scalar temperature,
                                        Scalar pg,
                                        FluidState &fluidState)
    {
        // ideal gas in both components!
        fluidState.setPartialPressure(H2OIdx, pg*fluidState.moleFrac(gPhaseIdx, H2OIdx));
        fluidState.setPartialPressure(N2Idx, pg*fluidState.moleFrac(gPhaseIdx, N2Idx));
    }

    /*!
     * \brief Given all mole fractions in a phase, return the phase
     *        density [kg/m^3].
     */
    template <class FluidState>
    static Scalar phaseDensity(int phaseIdx,
                               Scalar temperature,
                               Scalar pressure,
                               const FluidState &fluidState)
    { 
        if (phaseIdx == lPhaseIdx)
            return H2O::liquidDensity(temperature,
                                      pressure);
        else if (phaseIdx == gPhaseIdx)
        {
            // assume ideal gas. we just need the mean molar mass
            Scalar meanMolarMass = 
                fluidState.moleFrac(gPhaseIdx, H2OIdx)*H2O::molarMass() +
                fluidState.moleFrac(gPhaseIdx, N2Idx)*N2::molarMass();

            return 
                IdealGas::density(meanMolarMass, 
                                  temperature, 
                                  pressure);
        };
        DUNE_THROW(Dune::InvalidStateException, "Invalid phase index " << phaseIdx);
    }

    /*!
     * \brief Return the viscosity of a phase.
     */
    template <class FluidState>
    static Scalar phaseViscosity(int phaseIdx,
                                 Scalar temperature,
                                 Scalar pressure,
                                 const FluidState &fluidState)
    { 
        if (phaseIdx == lPhaseIdx) {
            // assume pure water for the liquid phase
            return H2O::liquidViscosity(temperature, 
                                        pressure);
        }
        else {
            // assume pure nitrogen for the gas phase
            return N2::gasViscosity(temperature, 
                                    pressure);
        }
    } 

    /*!
     * \brief Returns the derivative of the equilibrium partial
     *        pressure \f$\partial p^\kappa_g / \partial x^\kappa_l\$
     *        to the mole fraction of a component in the liquid phase.
     *
     * For solutions with only traces in a solvent this boils down to
     * the inverse Henry constant for the solutes and the partial
     * pressure for the solvent.
     */
    static Scalar degasPressure(int compIdx,
                                Scalar temperature,
                                Scalar pressure)
    {        
        switch (compIdx) {
        case H2OIdx: return H2O::vaporPressure(temperature);
        case N2Idx: return BinaryCoeff::H2O_N2::henry(temperature);
        };
        DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
    }

    /*!
     * \brief Given a component's pressure and temperature, return its
     *        density in a phase [kg/m^3].
     */
    static Scalar componentDensity(int phaseIdx,
                                   int compIdx,
                                   Scalar temperature, 
                                   Scalar pressure)
    {
        if (phaseIdx == lPhaseIdx) {
            if (compIdx == H2OIdx)
                return H2O::liquidDensity(temperature, pressure);
            else if (compIdx == N2Idx)
                return N2::liquidDensity(temperature, pressure);
            DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
        }
        else if (phaseIdx == gPhaseIdx) {
            if (compIdx == H2OIdx) {
                return H2O::gasDensity(temperature, pressure);
            }
            else if (compIdx == N2Idx)
                return N2::gasDensity(temperature, pressure);
            DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
        }
        DUNE_THROW(Dune::InvalidStateException, "Invalid phase index " << phaseIdx);
    };

    /*!
     * \brief Given a component's density and temperature, return the
     *        corresponding pressure in a phase [Pa].
     */
    static Scalar componentPressure(int phaseIdx,
                                    int compIdx,
                                    Scalar temperature, 
                                    Scalar density)
    {
        if (phaseIdx == lPhaseIdx) {
            if (compIdx == H2OIdx)
                return H2O::liquidPressure(temperature, density);
            else if (compIdx == N2Idx)
                return N2::liquidPressure(temperature, density);
            DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
        }
        else if (phaseIdx == gPhaseIdx) {
            if (compIdx == H2OIdx)
                return H2O::gasPressure(temperature, density);
            else if (compIdx == N2Idx)
                return N2::gasPressure(temperature, density);
            DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
        }
        DUNE_THROW(Dune::InvalidStateException, "Invalid phase index " << phaseIdx);
    };

    /*!
     * \brief Given all mole fractions, return the diffusion
     *        coefficent of a component in a phase.
     */
    template <class FluidState>
    static Scalar diffCoeff(int phaseIdx,
                            int compIIdx,
                            int compJIdx,
                            Scalar temperature,
                            Scalar pressure,
                            const FluidState &fluidState)
    { 
        if (compIIdx > compJIdx)
            std::swap(compIIdx, compJIdx);
        
        switch (phaseIdx) {
        case lPhaseIdx:
            switch (compIIdx) {
            case H2OIdx:
                switch (compJIdx) {
                case N2Idx: return BinaryCoeff::H2O_N2::liquidDiffCoeff(temperature, 
                                                                        pressure);
                }
            default:
                DUNE_THROW(Dune::InvalidStateException, 
                           "Binary diffusion coefficients of trace "
                           "substances in liquid phase is undefined!\n");
            }
        case gPhaseIdx:
            switch (compIIdx) {
            case H2OIdx:
                switch (compJIdx) {
                case N2Idx: return BinaryCoeff::H2O_N2::gasDiffCoeff(temperature,
                                                                     pressure);
                }
            }
        }

        DUNE_THROW(Dune::InvalidStateException, 
                   "Binary diffusion coefficient of components " 
                   << compIIdx << " and " << compJIdx
                   << " in phase " << phaseIdx << " is undefined!\n");
    };

    /*!
     * \brief Given all mole fractions in a phase, return the specific
     *        phase enthalpy [J/kg].
     */
    template <class FluidState>
    static Scalar phaseEnthalpy(int phaseIdx,
                                Scalar temperature,
                                Scalar pressure,
                                const FluidState &fluidState)
    { 
        if (phaseIdx == lPhaseIdx) {
            Scalar temperature = temperature;
            Scalar pressure = pressure;
            
            return 
                //fluidState.massFrac(lPhaseIdx, H2OIdx)*
                H2O::liquidEnthalpy(temperature, pressure);
        }
        else {
            Scalar pWater = fluidState.partialPressure(H2OIdx);

            Scalar result = 0;
            result += 
                H2O::gasEnthalpy(temperature, pWater) *
                fluidState.massFrac(gPhaseIdx, H2OIdx);
            result +=  
                N2::gasEnthalpy(temperature, 
                                fluidState.partialPressure(N2Idx)) *
                fluidState.massFrac(gPhaseIdx, N2Idx);
            
            return result;
        }
    }

    /*!
     * \brief Given all mole fractions in a phase, return the phase's
     *        internal energy [J/kg].
     */
    template <class FluidState>
    static Scalar phaseInternalEnergy(int phaseIdx,
                                      Scalar temperature,
                                      Scalar pressure,
                                      const FluidState &fluidState)
    { 
        Scalar h =
            enthalpy(phaseIdx, 
                     temperature, 
                     pressure,
                     fluidState);
            
        if (phaseIdx == lPhaseIdx) 
            return h - pressure*phaseDensity(phaseIdx, 
                                             temperature, 
                                             pressure,
                                             fluidState);
        else {
            // R*T == pressure * spec. volume for an ideal gas
            return h - IdealGas::R*temperature;
        }
    }
};

} // end namepace

#endif