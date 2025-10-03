'''
# Quaoar RIngs Occultation Simulator
**Basic Idea**

Generate an aperture with a ring segment cutting through it. Vary to radial profile of the ring, then calculate the Observers' Plane Intensity Field using Trester's Fourier Optics approach.

EFY, *SWRI*, 28-AUG-2025
From email on 02 Sept 2025:

Here's a notebook that generates a lightcurve for a Quaoar ring. You can set up any radial ring profile that you want. The notebook currently sets up four 46-km-wide profiles, where the optical depth is
- Flat across the ring with an optical depth of 0.1 (not very opaque)
- Flat across the ring with an optical dpeth of 1.0 (more opaque)
- Centrally peaked optical depth: zero at the edges, rising to 0.1 in the center
- Peaked at the edges (tau=0.1), but less optical depth at the center (tau=0)

The notebook includes three routines:
- occ_lc(): generates the observers' plane intensity field. 
  Short and sweet, following Trester (1999), in which the observers' plane E-field is the Fourier transform of
  a *modified* aperture function -- see his Eq. 3.
- solid_ap(): generates an aperture of an elliptical solid body. Not used in this notebook.
- RingSeg_ap(): generates an aperture spanning a segment of a ring.

Be aware that the Field of View and the Grid Sampling Size are a function of the wavelength, the distance to Quaoar and the extent of the array. In this notebook, the wavelength os 0.5 microns, the distance is 43 AU and the number of points on a side of the aperture array is 4096. That gives us a field of view that is 114 km across.

You'll see that ring profiles with sharp edges have lots of ringing, while the one case with soft edges (the centrally peaked case, in green) has no ringing.

PS - I've attached the notebook as HTML (read-only) and ipynb formats. I've been having trouble with inline graphics after a recent upgrade, so I'm using ipympl as a graphical backend. Feel free to substitute your favorite backend.
'''
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import fft2, ifft2, fftshift
from scipy.interpolate import interp1d
import time

#-----------------------------------------------------

def occ_lc(ap):
    '''Calculates the E-field in the observers plane for an occultation by an object with a complex aperture (ap).'''
    
    # Basic Idea:
    # This routine uses Trester's modified aperture function to calculate the E-field
    # in the far field. The modified aperture function is simply the complex aperture
    # times exp((ik/(2z0)) * (x0**2 + y0**2)). Since k = 2pi/lam and npts = lam*z0, this
    # exponential term is equivalent to exp((i*pi/npts) * (x0**2 + y0**2)).   
    
    npts = ap.shape[0] # Assume a square aperture
    n2 = int(npts/2)

    (y,x) = np.indices((npts,npts)) - 1.0*n2
    eTerm = np.exp(((np.pi * 1j)/npts)*((y)**2 + (x)**2))
    M = ap*eTerm
        
    E_obs = fft2(M)
    pObs = np.real(E_obs * np.conj(E_obs))
    
    return(np.roll(np.roll(pObs, n2, 0), n2, 1))

#-----------------------------------------------------, and her management referred her to Division 1

def solid_ap(FOV, npts, a, b):
    '''
    Makes apertures with elliptical obscurations. Platescale (km/pixel) is FOV/npts
    EFY, SwRI, 3-JUN-2013
    
    FOV	- Field of view (km)
    a,b	- Aperture axes (km)
    npts	- number of points across the FOV
    '''
    FOV2 = 0.5 * FOV
    mpts = np.linspace(-FOV2, FOV2, npts)
    xmesh,ymesh = np.meshgrid(mpts, mpts)
    rmesh = np.sqrt(xmesh**2 + ymesh**2)
    
    inAp = np.where(((ymesh/b)**2 + (xmesh/a)**2) < 1.0)
	
    ap = np.ones((npts, npts))
    ap[inAp] *= 0.
    return (ap)

#-----------------------------------------------------

def RingSeg_ap(lam, D, npts, wid, t_r):
    '''
    Makes an aperture with a vertical ring segment running through it. Note that the 
    FOV is sqrt(lam*D*npts), and the grid resolution is sqrt(lam*D/npts).

    INPUTS:
    lam - wavelength (microns)
    D - Distance from observer to occulting object (km)
    npts	- number of points across the array
    wid - width of the ring (km)
    t_r - a function that returns transmission as a function of distance from the ring's radial center

    OUTPUTS:
    ap - The 2D (npts x npts) aperture
    FOV - The field of view (km)
    gridSz - The size of each array element (km)

    EFY, SWRI, 28-AUG-2025
    '''
    lam_km = lam * 1e-9 # convert microns to km

    gridSz = np.sqrt(lam_km * D/npts)
    FOV = np.sqrt(lam_km * D * npts)
    FOV2 = 0.5 * FOV
    npts2 = int(0.5 * npts)

    wid2 = 0.5 * wid # Half the ring width

    # Make a grid of x values, centered on zero.
    xv = (np.arange(npts) - npts2) * gridSz # Column values (km), centered on the ring
    yv = np.ones(npts)
    grid = np.outer(yv, xv)

    # Mark the columns that are within +/- wid2 of the center
    inAp = np.abs(grid) <= wid2

    # Replace the in-aperture columns with their transmission functions
    ap = np.ones((npts,npts)) # One = full transmission, zero = blocked.
    ap[inAp] = t_r(grid[inAp])

    return (ap, FOV, gridSz)

#-----------------------------------------------------
# Make a transmission function to describe the radial profile of the ring

# We'll generate optical depths (tau) as a function of radius, then calculate
# transmissions as T = exp(-tau)

# Let's generate three type of radial profiles in optical depth:
# - flat across the ring's entire radius
# - tau up: sloping edges, peaked in the middle
# - tau down: sharp edges, low in the middle

wid = 46 # Width of the ring (km)
wid2 = 0.5 * wid

nRad = 100 # Number of steps across thering's radius
wVal = np.linspace(-wid2, wid2, nRad, endpoint=True) # points across the ring, from -wid/2 to +wid/2

# Generate some FLAT optical depth profiles across the ring's width
tauFlat01 = 0.1 * np.ones(nRad) # Flat profile, tau = 0.1
tauFlat10 = 1.0 * np.ones(nRad) # Flat profile, tau = 1.0

# Now some profiles with a parabolic radial profile
pb = wVal**2 # Make a parabola across the width of the ring
pbMax = pb.max()
pbMin = pb.min()

# Scale the parabola to the target max and min tau values
# targMax = m * pbMax + b
# targMin = m * pbMin + b
targMax = 0.0
targMin = 0.1

m = (targMax - targMin)/(pbMax - pbMin)
b = targMax - m * pbMax
tauCP = m * pb + b 


# Scale the parabola to the target max and min tau values
# targMax = m * pbMax + b
# targMin = m * pbMin + b
targMax = 0.1
targMin = 0.0
m = (targMax - targMin)/(pbMax - pbMin)
b = targMax - m * pbMax
tauSE = m * pb + b 

# Now build the interpolating functions
tF1 = interp1d(wVal, np.exp(-tauFlat01))
tF2 = interp1d(wVal, np.exp(-tauFlat10))

tCP = interp1d(wVal, np.exp(-tauCP))

tSE = interp1d(wVal, np.exp(-tauSE))
plt.figure(0)
plt.clf()
plt.plot(wVal, tauFlat01)
plt.plot(wVal, tauFlat10)
plt.plot(wVal, tauCP)
plt.plot(wVal, tauSE)
plt.xlabel('Radial Ring Position from Midpoint (km)')
plt.ylabel('Optical Depth')
# Build the transmission profiles: tr = exp(-tau)

trF1 = np.exp(-tF1(wVal))
trF2 = np.exp(-tF2(wVal))
trCP = np.exp(-tCP(wVal))
trSE = np.exp(-tSE(wVal))
plt.figure(1)
plt.clf()
plt.plot(wVal, tF1(wVal))
plt.plot(wVal, tF2(wVal))
plt.plot(wVal, tCP(wVal))
plt.plot(wVal, tSE(wVal))
plt.xlabel('Radial Ring Position from Midpoint (km)')
plt.ylabel('Transmitted Light Fraction')
lam = 0.5 # wavelength (microns)
D = 43 * 150e6 # Quaoar at 43 AU (km)
npts = 4096

apF1, FOV, gridSz = RingSeg_ap(lam, D, npts, wid, tF1)
apF2, FOV, gridSz = RingSeg_ap(lam, D, npts, wid, tF2)
apF3, FOV, gridSz = RingSeg_ap(lam, D, npts, wid, tCP)
apF4, FOV, gridSz = RingSeg_ap(lam, D, npts, wid, tSE)
opF1 = occ_lc(apF1)
opF2 = occ_lc(apF2)
opF3 = occ_lc(apF3)
opF4 = occ_lc(apF4)

# Normalize to an out-of-event baseline of 1
bg1 = np.median(opF1[2048, 0:100])
bg2 = np.median(opF2[2048, 0:100])
bg3 = np.median(opF3[2048, 0:100])
bg4 = np.median(opF4[2048, 0:100])

opF1 = opF1/bg1
opF2 = opF2/bg2
opF3 = opF3/bg3
opF4 = opF4/bg4
r_km = gridSz * (np.arange(npts) - npts/2)
v_event = 5.8 # Event velocity in km/s
t_sec = r_km/v_event

plt.figure(2)
plt.clf()
plt.plot(t_sec, opF1[2048,:]) # Blue
plt.plot(t_sec, opF2[2048,:]) # Orange
plt.plot(t_sec, opF3[2048,:]) # Green
plt.plot(t_sec, opF4[2048,:]) # Red
plt.xlabel('Time from mid-ring (s)')
plt.ylabel('Ring Occ. Lightcurve: Normalized Stellar Flux')

print(FOV, gridSz) # Field of View, grid sample size (km) 

#FOV is sqrt(lam*D*npts), and the grid resolution is sqrt(lam*D/npts).
# Try a wider ring, less optical depth.
# Make a transmission function to describe the radial profile of the ring

# We'll generate optical depths (tau) as a function of radius, then calculate
# transmissions as T = exp(-tau)

# Let's generate three type of radial profiles in optical depth:
# - flat across the ring's entire radius
# - tau up: sloping edges, peaked in the middle
# - tau down: sharp edges, low in the middle

widW = 100 # Width of the ring (km)
wid2 = 0.5 * widW

nRad = 200 # Number of steps across the ring's radius
wVal = np.linspace(-wid2, wid2, nRad, endpoint=True) # points across the ring, from -wid/2 to +wid/2

# Generate some FLAT optical depth profiles across the ring's width
tauFlat01 = 0.1 * np.ones(nRad) # Flat profile, tau = 0.1

# Now build the interpolating functions
tF1W = interp1d(wVal, np.exp(-tauFlat01))

plt.figure(0)
plt.clf()
plt.plot(wVal, tauFlat01)
plt.xlabel('Radial Ring Position from Midpoint (km)')
plt.ylabel('Optical Depth')
lam = 0.5 # wavelength (microns)
D = 43 * 150e6 # Quaoar at 43 AU (km)

start_time = time.time()
npts = 4096 # Increase either this or the t_sec below to get longer time/spatial plot. Increases compute time.

apF1, FOV, gridSz = RingSeg_ap(lam, D, npts, widW, tF1W)

opF1 = occ_lc(apF1)
# Normalize to an out-of-event baseline of 1
bg1 = np.median(opF1[2048, 0:100])
opF1 = opF1/bg1


print("--- %s seconds ---" % (time.time() - start_time))
r_km = gridSz * (np.arange(npts) - npts/2)
v_event = 5.8 # Event velocity in km/s
t_sec = r_km/v_event  # Fixed to be steps at this timing resolution. Can increase this or npts to cover more time.

plt.figure(2)
plt.clf()
plt.plot(t_sec, opF1[2048,:]) # Blue
plt.xlabel('Time from mid-ring (s)')
plt.ylabel('Ring Occ. Lightcurve: Normalized Stellar Flux')
lam = 0.5 # wavelength (microns)
D = 43 * 150e6 # Quaoar at 43 AU (km)

start_time = time.time()
npts = 4096*2 # Increase either this or the t_sec below to get longer time/spatial plot. Increases compute time.

apF1, FOV, gridSz = RingSeg_ap(lam, D, npts, widW, tF1W)

opF1 = occ_lc(apF1)
# Normalize to an out-of-event baseline of 1
bg1 = np.median(opF1[2048, 0:100])
opF1 = opF1/bg1


print("--- %s seconds ---" % (time.time() - start_time))
r_km = gridSz * (np.arange(npts) - npts/2)
v_event = 5.8 # Event velocity in km/s
t_sec = r_km/v_event  # Fixed to be steps at this timing resolution. Can increase this or npts to cover more time.

plt.figure(2)
plt.clf()
plt.plot(t_sec, opF1[2048,:]) # Blue
plt.xlabel('Time from mid-ring (s)')
plt.ylabel('Ring Occ. Lightcurve: Normalized Stellar Flux')
print(FOV, gridSz) # Field of View, grid sample size (km) 
lam = 0.5 # wavelength (microns)
D = 43 * 150e6 # Quaoar at 43 AU (km)

start_time = time.time()
npts = 4096*3 # Increase either this or the t_sec below to get longer time/spatial plot. Increases compute time.

apF1, FOV, gridSz = RingSeg_ap(lam, D, npts, widW, tF1W)

opF1 = occ_lc(apF1)
# Normalize to an out-of-event baseline of 1
bg1 = np.median(opF1[2048, 0:100])
opF1 = opF1/bg1


print("--- %s seconds ---" % (time.time() - start_time))
r_km = gridSz * (np.arange(npts) - npts/2)
v_event = 5.8 # Event velocity in km/s
t_sec = r_km/v_event  # Fixed to be steps at this timing resolution. Can increase this or npts to cover more time.

plt.figure(2)
plt.clf()
plt.plot(t_sec, opF1[2048,:]) # Blue
plt.xlabel('Time from mid-ring (s)')
plt.ylabel('Ring Occ. Lightcurve: Normalized Stellar Flux')
print(FOV, gridSz) # Field of View, grid sample size (km) 
