/* Copyright (C) 2010 Carlo de Falco

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/


#include <iostream>
#include <octave/oct.h>
#include <octave/oct-map.h>
#include <lo-ieee.h>

// ABSTRACT BASE CLASSES

class geopdes_mesh_base
{
protected:
  octave_idx_type nqn_rep, nel_rep, ndir_rep;
public:
  octave_idx_type nqn  (void) const { return nqn_rep; }
  octave_idx_type nel  (void) const { return nel_rep; }
  octave_idx_type ndir (void) const { return ndir_rep; }
  virtual double jacdet  (octave_idx_type inode, octave_idx_type iel) const = 0;
  virtual double weights (octave_idx_type inode, octave_idx_type iel) const = 0;
  inline double area (octave_idx_type iel) const {
    double a = 0.0;
    for (octave_idx_type iqn(0); iqn < nqn (); iqn++)
      a += std::abs ((jacdet (iqn, iel)) * (weights (iqn, iel)));
    return a;
  }
  inline double volume (octave_idx_type iel) const { return area (iel); }
};

class geopdes_space_base
{
protected:
  octave_idx_type ndof_rep, nsh_max_rep, ncomp_rep;

public:
  octave_idx_type ndof    (void) const { return ndof_rep;    }
  octave_idx_type nsh_max (void) const { return nsh_max_rep; }
  octave_idx_type ncomp   (void) const { return ncomp_rep;   }

  virtual octave_idx_type nsh (octave_idx_type iel) const = 0; 
  virtual octave_idx_type connectivity (octave_idx_type ish, octave_idx_type iel) const = 0;

  virtual double shape_functions (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m) const = 0;
  virtual double shape_function_gradients (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m, octave_idx_type n) const {return octave_NaN;};
  virtual double shape_function_curls (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m) const {return octave_NaN;};
  virtual double shape_function_divs  (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m) const {return octave_NaN;};
};

// PRELOADED MSH AND SPACE CLASSES

class geopdes_mesh: public geopdes_mesh_base
{
protected:
  Octave_map * msh;
  Matrix jacdet_rep, weights_rep;

public:
  geopdes_mesh (const Octave_map& refmsh)   
  { 
    msh      = new Octave_map (refmsh); 
    nqn_rep  = msh->contents  ("nqn")(0).int_value ();
    nel_rep  = msh->contents  ("nel")(0).int_value ();
    ndir_rep = msh->contents  ("quad_nodes")(0).array_value ().rows (); 
    jacdet_rep  = msh->contents ("jacdet")(0).matrix_value (); 
    weights_rep = msh->contents ("quad_weights")(0).matrix_value (); 
  }

  double jacdet  (octave_idx_type inode, octave_idx_type iel) const { return jacdet_rep  (inode, iel); }
  double weights (octave_idx_type inode, octave_idx_type iel) const { return weights_rep (inode, iel); }

};

class geopdes_mesh_normal: public geopdes_mesh
{
protected: 
  NDArray normal_rep;

public:
  geopdes_mesh_normal (const Octave_map& refmsh): geopdes_mesh (refmsh)
  {
    normal_rep = msh->contents ("normal")(0).array_value ();
  }

  double normal (octave_idx_type i, octave_idx_type inode, octave_idx_type iel) const {return normal_rep (i, inode, iel); }
};


class geopdes_space: public geopdes_space_base, protected geopdes_mesh
{
protected:
  Octave_map * sp;
  double * shape_functions_rep, 
    * shape_function_gradients_rep, 
    * shape_function_curls_rep, 
    * shape_function_divs_rep;
  Array<octave_idx_type> nsh_rep, connectivity_rep;
public:
  geopdes_space (const Octave_map& refsp, const geopdes_mesh& msh): geopdes_mesh (msh) 
  { 
    sp = new Octave_map (refsp); 
    ndof_rep    = sp->contents ("ndof")(0).int_value (); 
    nsh_max_rep = sp->contents ("nsh_max")(0).int_value (); 
    ncomp_rep   = sp->contents ("ncomp")(0).int_value (); 
    nsh_rep             = sp->contents ("nsh")(0).array_value (); 
    connectivity_rep    = sp->contents ("connectivity")(0).array_value ();

    shape_functions_rep = (double *) (sp->contents ("shape_functions")(0).array_value ()).data (); 
    shape_function_gradients_rep = NULL;
    if (sp->contains ("shape_function_gradients"))
      shape_function_gradients_rep = (double *) (sp->contents ("shape_function_gradients")(0).array_value ()).data (); 

    shape_function_curls_rep = NULL;
    if (sp->contains ("shape_function_curls"))
      shape_function_curls_rep = (double *) (sp->contents ("shape_function_curls")(0).array_value ()).data (); 

    shape_function_divs_rep = NULL;
    if (sp->contains ("shape_function_divs"))
      shape_function_divs_rep = (double *) (sp->contents ("shape_function_divs")(0).array_value ()).data ();         

  }

  octave_idx_type nsh (octave_idx_type iel) const { return nsh_rep (iel); }
  octave_idx_type connectivity (octave_idx_type ish, octave_idx_type iel) const { return connectivity_rep (ish, iel); }

  inline double shape_functions (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m) const {
    return shape_functions_rep[i + ncomp () * (j + nqn () * (k + nsh_max () * m))];
  }


  inline double shape_function_curls (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m) const {
    double res = (shape_function_curls_rep != NULL) ? shape_function_curls_rep [i + ncomp () * (j + nqn () * (k + nsh_max () * m))] : octave_NaN;
    return res;
  }
  
  inline double shape_function_curls (octave_idx_type i, octave_idx_type j, octave_idx_type k) const {
    double res = (shape_function_curls_rep != NULL) ? shape_function_curls_rep [i + nqn () * (j + nsh_max () * k)] : octave_NaN;
    return res;
  }


  inline double shape_function_divs (octave_idx_type j, octave_idx_type k, octave_idx_type m) const {
    double res = (shape_function_divs_rep != NULL) ? shape_function_divs_rep   [(j + nqn () * (k + nsh_max () * m))] : octave_NaN;
    return res;
  }
  
  inline double shape_function_gradients (octave_idx_type i, octave_idx_type j, octave_idx_type k, octave_idx_type m, octave_idx_type n) const {
    double res =  (shape_function_gradients_rep != NULL) ? shape_function_gradients_rep[i + ncomp () * (j + ndir () * (k + nqn () * (m + nsh_max () * n)))] : octave_NaN;
    return res;
  }

};
