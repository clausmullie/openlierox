/////////////////////////////////////////
//
//                  OpenLieroX
//
// code under LGPL, based on JasonBs work,
// enhanced by Dark Charlie and Albert Zeyer
//
//
/////////////////////////////////////////


// Game script class
// Created 7/2/02
// Jason Boettcher


#include <stdarg.h>

#include "EndianSwap.h"
#include "LieroX.h"
#include "Debug.h"
#include "FindFile.h"
#include "StringUtils.h"
#include "CGameScript.h"
#include "Error.h"
#include "ConfigHandler.h"


///////////////////
// Save the script (compiler)
int CGameScript::Save(const std::string& filename)
{
	FILE *fp;
	int n;


	// Open it
	fp = OpenGameFile(filename,"wb");
	if(fp == NULL) {
		errors << "CGameScript::Save: Could not open " << filename << " for writing" << endl;
		return false;
	}

	Header.Version = GS_VERSION;
	strcpy(Header.ID,"Liero Game Script");


	// Header
	gs_header_t tmpheader = Header;
	EndianSwap(tmpheader.Version);
	fwrite(&tmpheader,sizeof(gs_header_t),1,fp);

	fwrite(GetEndianSwapped(NumWeapons),sizeof(int),1,fp);

	// Weapons
	weapon_t *wpn = Weapons;

	for(n=0;n<NumWeapons;n++,wpn++) {

        writeString(wpn->Name, fp);
		fwrite(GetEndianSwapped(wpn->Type),          sizeof(int),    1, fp);

		// Special
		if(wpn->Type == WPN_SPECIAL) {
			fwrite(GetEndianSwapped(wpn->Special),   sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->tSpecial),  sizeof(gs_special_t), 1, fp); // WARNING: this works, because it contains only one int
			fwrite(GetEndianSwapped(wpn->Recharge),  sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->Drain),     sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->ROF),       sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->LaserSight),sizeof(int),    1, fp);
		}

		// Beam
		if(wpn->Type == WPN_BEAM) {
			fwrite(GetEndianSwapped(wpn->Recoil),    sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->Recharge),  sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->Drain),     sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->ROF),       sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->LaserSight),sizeof(int),    1, fp);

			fwrite(GetEndianSwapped(wpn->Bm_Colour[0]), sizeof(int), 1, fp);
			fwrite(GetEndianSwapped(wpn->Bm_Colour[1]), sizeof(int), 1, fp);
			fwrite(GetEndianSwapped(wpn->Bm_Colour[2]), sizeof(int), 1, fp);
			fwrite(GetEndianSwapped(wpn->Bm_Damage), sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->Bm_PlyDamage), sizeof(int), 1, fp);
			fwrite(GetEndianSwapped(wpn->Bm_Length), sizeof(int),    1, fp);
		}

		// Projectile
		if(wpn->Type == WPN_PROJECTILE) {
			fwrite(GetEndianSwapped(wpn->Class),     sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->Recoil),    sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->Recharge),  sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->Drain),     sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->ROF),       sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->ProjSpeed), sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->ProjSpeedVar),sizeof(float),1, fp);
			fwrite(GetEndianSwapped(wpn->ProjSpread),sizeof(float),  1, fp);
			fwrite(GetEndianSwapped(wpn->ProjAmount),sizeof(int),    1, fp);
			fwrite(GetEndianSwapped(wpn->LaserSight),sizeof(int),    1, fp);

			fwrite(GetEndianSwapped(wpn->UseSound),  sizeof(int),    1, fp);
			if(wpn->UseSound)
                writeString(wpn->SndFilename, fp);

			// Recursively save the projectile id's
			SaveProjectile(wpn->Projectile,fp);
		}
	}

	// Extra stuff


	// Ninja Rope
	fwrite(GetEndianSwapped(RopeLength),sizeof(int),1,fp);
	fwrite(GetEndianSwapped(RestLength),sizeof(int),1,fp);
	fwrite(GetEndianSwapped(Strength),sizeof(float),1,fp);

	// Worm
	gs_worm_t tmpworm = Worm;
	EndianSwap(tmpworm.AngleSpeed);
	EndianSwap(tmpworm.GroundSpeed);
	EndianSwap(tmpworm.AirSpeed);
	EndianSwap(tmpworm.Gravity);
	EndianSwap(tmpworm.JumpForce);
	EndianSwap(tmpworm.AirFriction);
	EndianSwap(tmpworm.GroundFriction);
	fwrite(&tmpworm, sizeof(gs_worm_t), 1, fp);


	fclose(fp);

	return true;
}


///////////////////
// Save a projectile
int CGameScript::SaveProjectile(proj_t *proj, FILE *fp)
{
	if(!proj)
		return false;

	fwrite(GetEndianSwapped(proj->Type),			sizeof(int),1,fp);
	fwrite(GetEndianSwapped(proj->Timer_Time),	sizeof(float),1,fp);
	fwrite(GetEndianSwapped(proj->Timer_TimeVar),sizeof(float),1,fp);
	fwrite(GetEndianSwapped(proj->Trail),		sizeof(int),1,fp);
	fwrite(GetEndianSwapped(proj->UseCustomGravity), sizeof(int), 1, fp);

	if(proj->UseCustomGravity)
		fwrite(GetEndianSwapped(proj->Gravity),	sizeof(int), 1, fp);

	fwrite(GetEndianSwapped(proj->Dampening),	sizeof(int), 1, fp);

    // Pixel type
	if(proj->Type == PRJ_PIXEL) {
		fwrite(GetEndianSwapped(proj->NumColours), sizeof(int), 1, fp);
		fwrite(GetEndianSwapped(proj->Colour1[0]),   sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Colour1[1]),   sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Colour1[2]),   sizeof(int),1,fp);
		if(proj->NumColours == 2)
		{
			fwrite(GetEndianSwapped(proj->Colour2[0]),   sizeof(int),1,fp);
			fwrite(GetEndianSwapped(proj->Colour2[1]),   sizeof(int),1,fp);
			fwrite(GetEndianSwapped(proj->Colour2[2]),   sizeof(int),1,fp);
		}
	}

    // Image type
	else if(proj->Type == PRJ_IMAGE) {
        writeString(proj->ImgFilename, fp);
		fwrite(GetEndianSwapped(proj->Rotating), sizeof(int), 1, fp);
		fwrite(GetEndianSwapped(proj->RotIncrement), sizeof(int), 1, fp);
		fwrite(GetEndianSwapped(proj->RotSpeed), sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->UseAngle), sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->UseSpecAngle),sizeof(int),1,fp);
		if(proj->UseAngle || proj->UseSpecAngle)
			fwrite(GetEndianSwapped(proj->AngleImages),sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Animating),sizeof(int),1,fp);
		if(proj->Animating) {
			fwrite(GetEndianSwapped(proj->AnimRate),sizeof(float),1,fp);
			fwrite(GetEndianSwapped(proj->AnimType),sizeof(int),1,fp);
		}
	}




	//
	// Hit
	//
	fwrite(GetEndianSwapped(proj->Hit_Type),sizeof(int),1,fp);

	// Hit::Explode
	if(proj->Hit_Type == PJ_EXPLODE) {
		fwrite(GetEndianSwapped(proj->Hit_Damage),		sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Hit_Projectiles),	sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Hit_UseSound),		sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->Hit_Shake),		sizeof(int),1,fp);
		if(proj->Hit_UseSound)
            writeString(proj->Hit_SndFilename, fp);
	}

	// Hit::Bounce
	if(proj->Hit_Type == PJ_BOUNCE) {
		fwrite(GetEndianSwapped(proj->Hit_BounceCoeff),	sizeof(float),	1,fp);
		fwrite(GetEndianSwapped(proj->Hit_BounceExplode),sizeof(int),	1,fp);
	}

	// Hit::Carve
	if(proj->Hit_Type == PJ_CARVE) {
		fwrite(GetEndianSwapped(proj->Hit_Damage),		sizeof(int),1,fp);
	}





	//
	// Timer
	//
	if(proj->Timer_Time > 0) {
		fwrite(GetEndianSwapped(proj->Timer_Type),sizeof(int),1,fp);
		if(proj->Timer_Type == PJ_EXPLODE) {
			fwrite(GetEndianSwapped(proj->Timer_Damage),		sizeof(int),1,fp);
			fwrite(GetEndianSwapped(proj->Timer_Projectiles),sizeof(int),1,fp);
			fwrite(GetEndianSwapped(proj->Timer_Shake),sizeof(int),1,fp);
		}
	}


	//
	// Player hit
	//
	fwrite(GetEndianSwapped(proj->PlyHit_Type),sizeof(int),1,fp);

	// PlyHit::Explode || PlyHit::Injure
	if(proj->PlyHit_Type == PJ_EXPLODE || proj->PlyHit_Type == PJ_INJURE) {
		fwrite(GetEndianSwapped(proj->PlyHit_Damage),sizeof(int),1,fp);
		fwrite(GetEndianSwapped(proj->PlyHit_Projectiles),sizeof(int),1,fp);
	}

	// PlyHit::Bounce
	if(proj->PlyHit_Type == PJ_BOUNCE) {
		fwrite(GetEndianSwapped(proj->PlyHit_BounceCoeff),sizeof(float),1,fp);
	}


    //
    // Explode
    //
    fwrite(GetEndianSwapped(proj->Exp_Type),     sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Exp_Damage),   sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Exp_Projectiles), sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Exp_UseSound), sizeof(int), 1, fp);
    if(proj->Exp_UseSound)
        writeString(proj->Exp_SndFilename, fp);


    //
    // Touch
    //
    fwrite(GetEndianSwapped(proj->Tch_Type),     sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Tch_Damage),   sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Tch_Projectiles), sizeof(int), 1, fp);
    fwrite(GetEndianSwapped(proj->Tch_UseSound), sizeof(int), 1, fp);
    if(proj->Tch_UseSound)
        writeString(proj->Tch_SndFilename, fp);



	if(proj->Timer_Projectiles || proj->Hit_Projectiles || proj->PlyHit_Projectiles || proj->Exp_Projectiles ||
       proj->Tch_Projectiles) {
		fwrite(GetEndianSwapped(proj->ProjUseangle),	sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->ProjAngle),	sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->ProjAmount),	sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->ProjSpread),	sizeof(float),	1, fp);
		fwrite(GetEndianSwapped(proj->ProjSpeed),	sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->ProjSpeedVar),	sizeof(float),	1, fp);

		SaveProjectile(proj->Projectile,fp);
	}


	// Projectile trail
	if(proj->Trail == TRL_PROJECTILE) {

		fwrite(GetEndianSwapped(proj->PrjTrl_UsePrjVelocity),	sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->PrjTrl_Delay),				sizeof(float),	1, fp);
		fwrite(GetEndianSwapped(proj->PrjTrl_Amount),			sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->PrjTrl_Speed),				sizeof(int),	1, fp);
		fwrite(GetEndianSwapped(proj->PrjTrl_SpeedVar),			sizeof(float),	1, fp);
		fwrite(GetEndianSwapped(proj->PrjTrl_Spread),			sizeof(float),	1, fp);

		SaveProjectile(proj->PrjTrl_Proj, fp);
	}

	return true;
}


///////////////////
// Load the game script from a file (game)
int CGameScript::Load(const std::string& dir)
{
	// Already cached externally
	/*
	// Try cache first
	CGameScript *cached = cCache.GetMod(dir);
	if (cached != NULL)  {
		CopyFrom(cached);
		return GSE_OK;
	}
	*/

	loaded = false;
	FILE *fp;
	int n;
	std::string filename = dir + "/script.lgs";
	sDirectory = dir;

	// Open it
	fp = OpenGameFile(filename,"rb");
	if(fp == NULL) {
		warnings << "CGameScript::Load(): Could not load file " << filename << endl;
		return GSE_FILE;
	}

	// Header
	fread(&Header,sizeof(gs_header_t),1,fp);
	EndianSwap(Header.Version);
	// for security
	fix_markend(Header.ID);
	fix_markend(Header.ModName);

	// Check ID
	if(strcmp(Header.ID,"Liero Game Script") != 0) {
		fclose(fp);
		SetError("CGameScript::Load(): Bad script id");
		return GSE_BAD;
	}

	// Check version
	if(Header.Version < GS_FIRST_SUPPORTED_VERSION || Header.Version > GS_VERSION) {
		warnings << "GS:CheckFile: WARNING: " << dir << "/script.lgs has the wrong version";
		warnings << " (" << (unsigned)Header.Version << ", required is in the range ";
		warnings << "[" << GS_FIRST_SUPPORTED_VERSION << "," << GS_VERSION << "])" << endl;
		fclose(fp);
		SetError("CGameScript::Load(): Bad script version");
		return GSE_VERSION;
	}

    // Clear an old mod file
    modLog("Loading game mod file " + filename);
	//modLog("  ID = %s", Header.ID);
	//modLog("  Version = %i", Header.Version);

	fread(&NumWeapons,sizeof(int),1,fp);
	EndianSwap(NumWeapons);
	//modLog("  NumWeapons = %i", NumWeapons);

	// Do Allocations
	Weapons = new weapon_t[NumWeapons];
	if(Weapons == NULL) {
		SetError("SGameScript::Load(): Out of memory");
		return GSE_MEM;
	}

	// Weapons
	weapon_t *wpn;
	for(n=0;n<NumWeapons;n++) {
		wpn = &Weapons[n];

		wpn->ID = n;
		wpn->Projectile = NULL;

        wpn->Name = readString(fp);
		fread(&wpn->Type,           sizeof(int),    1,fp);
		EndianSwap(wpn->Type);

		// Special
		if(wpn->Type == WPN_SPECIAL) {
			fread(&wpn->Special,    sizeof(int),    1, fp);
			EndianSwap(wpn->Special);
			fread(&wpn->tSpecial,   sizeof(gs_special_t), 1, fp);
			EndianSwap(wpn->tSpecial.Thrust);
			fread(&wpn->Recharge,   sizeof(float),  1, fp);
			EndianSwap(wpn->Recharge);
			fread(&wpn->Drain,      sizeof(float),  1, fp);
			EndianSwap(wpn->Drain);
			fread(&wpn->ROF,        sizeof(float),  1, fp);
			EndianSwap(wpn->ROF);
			fread(&wpn->LaserSight, sizeof(int),    1, fp);
			EndianSwap(wpn->LaserSight);
		}

		// Beam
		if(wpn->Type == WPN_BEAM) {
			fread(&wpn->Recoil,     sizeof(int),    1, fp);
			EndianSwap(wpn->Recoil);
			fread(&wpn->Recharge,   sizeof(float),  1, fp);
			EndianSwap(wpn->Recharge);
			fread(&wpn->Drain,      sizeof(float),  1, fp);
			EndianSwap(wpn->Drain);
			fread(&wpn->ROF,        sizeof(float),  1, fp);
			EndianSwap(wpn->ROF);
			fread(&wpn->LaserSight, sizeof(int),    1, fp);
			EndianSwap(wpn->LaserSight);

			fread(wpn->Bm_Colour, sizeof(int),      3, fp);
			EndianSwap(wpn->Bm_Colour[0]);
			EndianSwap(wpn->Bm_Colour[1]);
			EndianSwap(wpn->Bm_Colour[2]);
			fread(&wpn->Bm_Damage, sizeof(int),     1, fp);
			EndianSwap(wpn->Bm_Damage);
			fread(&wpn->Bm_PlyDamage, sizeof(int),  1, fp);
			EndianSwap(wpn->Bm_PlyDamage);
			fread(&wpn->Bm_Length, sizeof(int),     1, fp);
			EndianSwap(wpn->Bm_Length);
		}

		// Projectile
		if(wpn->Type == WPN_PROJECTILE) {

			fread(&wpn->Class,sizeof(int),1,fp);
			EndianSwap(wpn->Class);
			fread(&wpn->Recoil,sizeof(int),1,fp);
			EndianSwap(wpn->Recoil);
			fread(&wpn->Recharge,sizeof(float),1,fp);
			EndianSwap(wpn->Recharge);
			fread(&wpn->Drain,sizeof(float),1,fp);
			EndianSwap(wpn->Drain);
			fread(&wpn->ROF,sizeof(float),1,fp);
			EndianSwap(wpn->ROF);
			fread(&wpn->ProjSpeed,sizeof(int),1,fp);
			EndianSwap(wpn->ProjSpeed);
			fread(&wpn->ProjSpeedVar,sizeof(float),1,fp);
			EndianSwap(wpn->ProjSpeedVar);
			fread(&wpn->ProjSpread,sizeof(float),1,fp);
			EndianSwap(wpn->ProjSpread);
			fread(&wpn->ProjAmount,sizeof(int),1,fp);
			EndianSwap(wpn->ProjAmount);
			fread(&wpn->LaserSight, sizeof(int), 1, fp);
			EndianSwap(wpn->LaserSight);

			fread(&wpn->UseSound,sizeof(int),1,fp);
			EndianSwap(wpn->UseSound);
			if(wpn->UseSound) {
                wpn->SndFilename = readString(fp);

				// Load the sample
				wpn->smpSample = LoadGSSample(dir,wpn->SndFilename);
				if(wpn->smpSample == NULL)
					wpn->UseSound = false;
			}

			wpn->Projectile = LoadProjectile(fp);
		}

		wpn->ROF /= 1000.0f;
		wpn->Recharge /= 10.0f;
	}


	// Extra stuff


	// Ninja Rope
	fread(&RopeLength,sizeof(int),1,fp);
	EndianSwap(RopeLength);
	fread(&RestLength,sizeof(int),1,fp);
	EndianSwap(RestLength);
	fread(&Strength,sizeof(float),1,fp);
	EndianSwap(Strength);

	// Worm
	fread(&Worm, sizeof(gs_worm_t), 1, fp);
	EndianSwap(Worm.AngleSpeed);
	EndianSwap(Worm.GroundSpeed);
	EndianSwap(Worm.AirSpeed);
	EndianSwap(Worm.Gravity);
	EndianSwap(Worm.JumpForce);
	EndianSwap(Worm.AirFriction);
	EndianSwap(Worm.GroundFriction);


	fclose(fp);

	// Already cached externally
	// Save to cache
	//cCache.SaveMod(dir, this);

	loaded = true;
	
	return GSE_OK;
}


///////////////////
// Load a projectile
proj_t *CGameScript::LoadProjectile(FILE *fp)
{
	proj_t *proj = new proj_t;
	if(proj == NULL)
		return NULL;


	fread(&proj->Type,			sizeof(int),  1,fp);
	EndianSwap(proj->Type);
	fread(&proj->Timer_Time,	sizeof(float),1,fp);
	EndianSwap(proj->Timer_Time);
	fread(&proj->Timer_TimeVar,	sizeof(float),1,fp);
	EndianSwap(proj->Timer_TimeVar);
	fread(&proj->Trail,			sizeof(int),  1,fp);
	EndianSwap(proj->Trail);
	fread(&proj->UseCustomGravity, sizeof(int), 1, fp);
	EndianSwap(proj->UseCustomGravity);
	if(proj->UseCustomGravity)
	{
		fread(&proj->Gravity,	sizeof(int), 1, fp);
		EndianSwap(proj->Gravity);
	}
	fread(&proj->Dampening,		sizeof(int),  1, fp);
	EndianSwap(proj->Dampening);

	proj->PrjTrl_Proj = NULL;
	proj->Projectile = NULL;
	proj->Hit_Projectiles = false;
	proj->Timer_Projectiles = false;
	proj->PlyHit_Projectiles = false;
	proj->Animating = false;
	proj->Rotating = 0;
	proj->RotIncrement = 0;
	proj->Timer_Shake = 0;

	if(proj->Type == PRJ_PIXEL) {
		fread(&proj->NumColours, sizeof(int), 1, fp);
		EndianSwap(proj->NumColours);
		fread(proj->Colour1,sizeof(int),3,fp);
		EndianSwap(proj->Colour1[0]);
		EndianSwap(proj->Colour1[1]);
		EndianSwap(proj->Colour1[2]);
		if(proj->NumColours == 2)
		{
			fread(proj->Colour2,sizeof(int),3,fp);
			EndianSwap(proj->Colour2[0]);
			EndianSwap(proj->Colour2[1]);
			EndianSwap(proj->Colour2[2]);
		}

	}
	else if(proj->Type == PRJ_IMAGE) {
        proj->ImgFilename = readString(fp);

		proj->bmpImage = LoadGSImage(sDirectory, proj->ImgFilename);
        if(!proj->bmpImage)
            modLog("Could not open image '" + proj->ImgFilename + "'");

		fread(&proj->Rotating, sizeof(int), 1, fp);
		EndianSwap(proj->Rotating);
		fread(&proj->RotIncrement, sizeof(int), 1, fp);
		EndianSwap(proj->RotIncrement);
		fread(&proj->RotSpeed,sizeof(int),1,fp);
		EndianSwap(proj->RotSpeed);
		fread(&proj->UseAngle,sizeof(int),1,fp);
		EndianSwap(proj->UseAngle);
		fread(&proj->UseSpecAngle,sizeof(int),1,fp);
		EndianSwap(proj->UseSpecAngle);
		if(proj->UseAngle || proj->UseSpecAngle)
		{
			fread(&proj->AngleImages,sizeof(int),1,fp);
			EndianSwap(proj->AngleImages);
		}
		fread(&proj->Animating,sizeof(int),1,fp);
		EndianSwap(proj->Animating);
		if(proj->Animating) {
			fread(&proj->AnimRate,sizeof(float),1,fp);
			EndianSwap(proj->AnimRate);
			fread(&proj->AnimType,sizeof(int),1,fp);
			EndianSwap(proj->AnimType);
		}
	}


	//
	// Hit
	//
	fread(&proj->Hit_Type,sizeof(int),1,fp);
	EndianSwap(proj->Hit_Type);

	// Hit::Explode
	if(proj->Hit_Type == PJ_EXPLODE) {
		fread(&proj->Hit_Damage,		sizeof(int),1,fp);
		EndianSwap(proj->Hit_Damage);
		fread(&proj->Hit_Projectiles,	sizeof(int),1,fp);
		EndianSwap(proj->Hit_Projectiles);
		fread(&proj->Hit_UseSound,		sizeof(int),1,fp);
		EndianSwap(proj->Hit_UseSound);
		fread(&proj->Hit_Shake,			sizeof(int),1,fp);
		EndianSwap(proj->Hit_Shake);

		if(proj->Hit_UseSound) {
            proj->Hit_SndFilename = readString(fp);

			if(!bDedicated) {
				// Load the sample
				proj->smpSample = LoadGSSample(sDirectory,proj->Hit_SndFilename);

				if(proj->smpSample == NULL) {
					proj->Hit_UseSound = false;
					modLog("Could not open sound '" + proj->Hit_SndFilename + "'");
				}
			} else
				proj->smpSample = NULL;
		}
	}

	// Hit::Bounce
	if(proj->Hit_Type == PJ_BOUNCE) {
		fread(&proj->Hit_BounceCoeff,	sizeof(float), 1, fp);
		EndianSwap(proj->Hit_BounceCoeff);
		fread(&proj->Hit_BounceExplode, sizeof(int),   1, fp);
		EndianSwap(proj->Hit_BounceExplode);
	}

	// Hit::Carve
	if(proj->Hit_Type == PJ_CARVE) {
		fread(&proj->Hit_Damage,		sizeof(int),1,fp);
		EndianSwap(proj->Hit_Damage);
	}





	//
	// Timer
	//
	if(proj->Timer_Time > 0) {
		fread(&proj->Timer_Type,sizeof(int),1,fp);
		EndianSwap(proj->Timer_Type);
		if(proj->Timer_Type == PJ_EXPLODE) {
			fread(&proj->Timer_Damage,sizeof(int),1,fp);
			EndianSwap(proj->Timer_Damage);
			fread(&proj->Timer_Projectiles,sizeof(int),1,fp);
			EndianSwap(proj->Timer_Projectiles);
			fread(&proj->Timer_Shake,sizeof(int),1,fp);
			EndianSwap(proj->Timer_Shake);
		}
	}



	//
	// Player hit
	//
	fread(&proj->PlyHit_Type,sizeof(int),1,fp);
	EndianSwap(proj->PlyHit_Type);

	// PlyHit::Explode || PlyHit::Injure
	if(proj->PlyHit_Type == PJ_INJURE || proj->PlyHit_Type == PJ_EXPLODE) {
		fread(&proj->PlyHit_Damage,sizeof(int),1,fp);
		EndianSwap(proj->PlyHit_Damage);
		fread(&proj->PlyHit_Projectiles,sizeof(int),1,fp);
		EndianSwap(proj->PlyHit_Projectiles);
	}

	// PlyHit::Bounce
	if(proj->PlyHit_Type == PJ_BOUNCE) {
		fread(&proj->PlyHit_BounceCoeff, sizeof(float), 1, fp);
		EndianSwap(proj->PlyHit_BounceCoeff);
	}


    //
    // Explode
    //
    fread(&proj->Exp_Type,     sizeof(int), 1, fp);
	EndianSwap(proj->Exp_Type);
    fread(&proj->Exp_Damage,   sizeof(int), 1, fp);
	EndianSwap(proj->Exp_Damage);
    fread(&proj->Exp_Projectiles, sizeof(int), 1, fp);
	EndianSwap(proj->Exp_Projectiles);
    fread(&proj->Exp_UseSound, sizeof(int), 1, fp);
	EndianSwap(proj->Exp_UseSound);
    if(proj->Exp_UseSound)
    {
        proj->Exp_SndFilename = readString(fp);
	}

    //
    // Touch
    //
    fread(&proj->Tch_Type,     sizeof(int), 1, fp);
	EndianSwap(proj->Tch_Type);
    fread(&proj->Tch_Damage,   sizeof(int), 1, fp);
	EndianSwap(proj->Tch_Damage);
    fread(&proj->Tch_Projectiles, sizeof(int), 1, fp);
	EndianSwap(proj->Tch_Projectiles);
    fread(&proj->Tch_UseSound, sizeof(int), 1, fp);
	EndianSwap(proj->Tch_UseSound);
    if(proj->Tch_UseSound)
        proj->Tch_SndFilename = readString(fp);


	if(proj->Timer_Projectiles || proj->Hit_Projectiles || proj->PlyHit_Projectiles || proj->Exp_Projectiles ||
       proj->Tch_Projectiles) {
		fread(&proj->ProjUseangle,	sizeof(int),	1, fp);
		EndianSwap(proj->ProjUseangle);
		fread(&proj->ProjAngle,		sizeof(int),	1, fp);
		EndianSwap(proj->ProjAngle);
		fread(&proj->ProjAmount,	sizeof(int),	1, fp);
		EndianSwap(proj->ProjAmount);
		fread(&proj->ProjSpread,	sizeof(float),	1, fp);
		EndianSwap(proj->ProjSpread);
		fread(&proj->ProjSpeed,		sizeof(int),	1, fp);
		EndianSwap(proj->ProjSpeed);
		fread(&proj->ProjSpeedVar,	sizeof(float),	1, fp);
		EndianSwap(proj->ProjSpeedVar);

		proj->Projectile = LoadProjectile(fp);
	}

	// Projectile trail
	if(proj->Trail == TRL_PROJECTILE) {

		fread(&proj->PrjTrl_UsePrjVelocity,	sizeof(int),	1, fp);
		EndianSwap(proj->PrjTrl_UsePrjVelocity);
		fread(&proj->PrjTrl_Delay,			sizeof(float),	1, fp);
		EndianSwap(proj->PrjTrl_Delay);
		fread(&proj->PrjTrl_Amount,			sizeof(int),	1, fp);
		EndianSwap(proj->PrjTrl_Amount);
		fread(&proj->PrjTrl_Speed,			sizeof(int),	1, fp);
		EndianSwap(proj->PrjTrl_Speed);
		fread(&proj->PrjTrl_SpeedVar,		sizeof(float),	1, fp);
		EndianSwap(proj->PrjTrl_SpeedVar);
		fread(&proj->PrjTrl_Spread,			sizeof(float),	1, fp);
		EndianSwap(proj->PrjTrl_Spread);

		// Change from milli-seconds to seconds
		proj->PrjTrl_Delay /= 1000.0f;

		proj->PrjTrl_Proj = LoadProjectile(fp);
	}

	return proj;
}


///////////////////
// Load an image
SDL_Surface * CGameScript::LoadGSImage(const std::string& dir, const std::string& filename)
{
	SmartPointer<SDL_Surface> img = NULL;

	// First, check the gfx directory in the mod dir
	img = LoadGameImage(dir + "/gfx/" + filename, true);
	if(img.get())  {
		SetColorKey(img.get());
		CachedImages.push_back(img);
		return img.get();
	}

	// Check the gfx directory in the data dir
	img = LoadGameImage("data/gfx/" + filename, true);
	if(img.get())
	{ 
		SetColorKey(img.get());
		CachedImages.push_back(img);
	}
	return img.get();
}


///////////////////
// Load a sample
SoundSample * CGameScript::LoadGSSample(const std::string& dir, const std::string& filename)
{
	SmartPointer<SoundSample> smp = NULL;

	// First, check the sfx directory in the mod dir
	smp = LoadSample(dir + "/sfx/" + filename, 10);

	if(smp.get())
	{
		CachedSamples.push_back(smp);
		return smp.get();
	}

	// Check the sounds directory in the data dir
	smp = LoadSample("data/sounds/" + filename, 10);
	if(smp.get())
		CachedSamples.push_back(smp);
	return smp.get();
}



///////////////////
// Find a weapon based on its name
const weapon_t *CGameScript::FindWeapon(const std::string& name)
{
	int n;

	// Go through each weapon
	weapon_t *wpn = Weapons;
	for(n=0;n<NumWeapons;n++,wpn++) {

		if(stringcaseequal(wpn->Name, name))
			return wpn;
	}

	// Instead of returning NULL, just return the first weapon
	return Weapons;
}


///////////////////
// Returns true if the weapon is in the game script
bool CGameScript::weaponExists(const std::string& szName)
{
    // Go through each weapon
	weapon_t *wpn = Weapons;
	for(int n=0;n<NumWeapons;n++,wpn++) {

		if(stringcasecmp(wpn->Name, szName) == 0)
			return true;
	}

    // Not found
    return false;
}


///////////////////
// Write a string in pascal format
void CGameScript::writeString(const std::string& szString, FILE *fp)
{
    if(szString == "") return;

	size_t length = szString.size();
	if(length > 255) {
		warnings << "i will cut the following string for writing: " << szString << endl;
		length = 255;
	}
	uchar len = (uchar)length;

    fwrite( &len, sizeof(uchar), 1, fp );
    fwrite( szString.c_str(),sizeof(char), length, fp );
}


///////////////////
// Read a string in pascal format
std::string CGameScript::readString(FILE *fp)
{
	char buf[256];

    uchar length;
    fread( &length, sizeof(uchar), 1, fp );
    fread( buf,sizeof(char), length, fp );

    buf[length] = '\0';

    return buf;
}

// Helper function
static size_t GetProjSize(proj_t *prj)
{
	if (prj)
		return 	prj->Exp_SndFilename.size() + prj->filename.size() +
				prj->Hit_SndFilename.size() + prj->ImgFilename.size() +
				GetProjSize(prj->Projectile) + prj->Tch_SndFilename.size() +
				/*GetSurfaceMemorySize(prj->bmpImage.get()) + */
				sizeof(proj_t);
	else
		return 0;
}

//////////////////
// Returns the memory occupied by this gamescript
size_t CGameScript::GetMemorySize()
{
	size_t res = sizeof(CGameScript) + sDirectory.size();
	weapon_t *it = Weapons;
	for (int i = 0; i < NumWeapons; i++, it++)  {
		res += sizeof(weapon_t) + sizeof(SoundSample);
		res += it->SndFilename.size();
		res += GetProjSize(it->Projectile);
	}

	return res;
}


///////////////////
// Shutdown the game script
void CGameScript::Shutdown(void)
{
	loaded = false;
	int n;

    // Close the log file
    if(pModLog) {
        fclose(pModLog);
        pModLog = NULL;
    }

	if(Weapons == NULL)
		return;

	// Go through each weapon
	weapon_t *wpn;
	for(n=0;n<NumWeapons;n++) {
		wpn = &Weapons[n];
		// Shutdown any projectiles
		ShutdownProjectile(wpn->Projectile);
	}

	delete[] Weapons;
	Weapons = NULL;
}


///////////////////
// Shutdown a projectile
void CGameScript::ShutdownProjectile(proj_t *prj)
{
	if(prj) {
		ShutdownProjectile(prj->Projectile);
		ShutdownProjectile(prj->PrjTrl_Proj);
		delete prj;
	}
}


///////////////////
// Check if a file is a valid liero game script
bool CGameScript::CheckFile(const std::string& dir, std::string& name, bool abs_filename)
{
	name = "";

	// Open it
	FILE *fp = NULL;
	if(abs_filename) {
		std::string filename;
		// we still need to add "/script.lgs" and then do an exact filename search
		if(GetExactFileName(dir + "/script.lgs", filename))
	 		fp = fopen(filename.c_str(), "rb");
	} else
		fp = OpenGameFile(dir + "/script.lgs", "rb");
	if(fp == NULL) return false;

	// Header
	gs_header_t head;
	memset(&head,0,sizeof(gs_header_t));
	fread(&head,sizeof(gs_header_t),1,fp);
	fclose(fp);

	EndianSwap(head.Version);
	// for security
	fix_markend(head.ID);
	fix_markend(head.ModName);

	// Check ID
	if(strcmp(head.ID,"Liero Game Script") != 0) {
		warnings << "GS:CheckFile: WARNING: " << dir << "/script.lgs is not a Liero game script";
		warnings << " (but \"" << head.ID << "\" instead)" << endl;
		return false;
	}

	// Check version
	if(head.Version < GS_FIRST_SUPPORTED_VERSION || head.Version > GS_VERSION) {
		warnings << "GS:CheckFile: WARNING: " << dir << "/script.lgs has the wrong version";
		warnings << " (" << (unsigned)head.Version << ", required is in the range ";
		warnings << "[" << GS_FIRST_SUPPORTED_VERSION << "," << GS_VERSION << "])" << endl;
		return false;
	}

	name = head.ModName;
	return true;
}


///////////////////
// Return an error message based on code
std::string CGameScript::getError(int code)
{
	std::string text = "Undefined error";

	switch(code) {

		case GSE_MEM:
			text = "Out of memory";
			break;

		case GSE_VERSION:
			text = "Incorrect version";
			break;

		case GSE_FILE:
			text = "Could not open file";
			break;

		case GSE_BAD:
			text = "Bad file format";
			break;
	}

	return text;
}


///////////////////
// Write info to a mod log file
void CGameScript::modLog(const std::string& text)
{
	notes << "modLog: " << text << endl;

	if(!pModLog) {
		pModLog = OpenGameFile("modlog.txt","wt");
		if(!pModLog)
			return;
		fprintf(pModLog,"Log file for mod:\n%s\n--------------------------------\n",Header.ModName);
	}

	if (text.size() != 0)
		fprintf(pModLog,"%s\n", text.c_str());
}

// Not needed with new caching system - game script won't ever change it's state during game
/*
///////////////////
// Copies infor from anothe gamescript
void CGameScript::CopyFrom(CGameScript *cg)
{
	sDirectory = cg->sDirectory;
	Header = cg->Header;
	NumWeapons = cg->NumWeapons;

	// HINT: only a pointer is copied here, because the weapon info does not change
	// so it would be wasting of memory if we copied the whole list
	Weapons = cg->Weapons;

	Worm = cg->Worm;
	RopeLength = cg->RopeLength;
	RestLength = cg->RestLength;
	Strength = cg->Strength;
}
*/

template <> void SmartPointer_ObjectDeinit<CGameScript> ( CGameScript * obj )
{
	delete obj;
}







CGameScript* Game = NULL;

int		CompileWeapon(const char* dir, const std::string& weapon, int id);
void	CompileBeam(const char* file, weapon_t *Weap);
proj_t  *CompileProjectile(const char* dir, const char* pfile);
int		CompileExtra(const char* dir);
int		CompileJetpack(const char* file, weapon_t *Weap);

int ProjCount = 0;

bool initCompiler() {
	Game = new CGameScript;
	if(Game == NULL) {
		errors << "initCompiler: Out of memory" << endl;
		return false;
	}

	// Add some keywords
	AddKeyword("WPN_PROJECTILE",WPN_PROJECTILE);
	AddKeyword("WPN_SPECIAL",WPN_SPECIAL);
	AddKeyword("WPN_BEAM",WPN_BEAM);
	AddKeyword("WCL_AUTOMATIC",WCL_AUTOMATIC);
	AddKeyword("WCL_POWERGUN",WCL_POWERGUN);
	AddKeyword("WCL_GRENADE",WCL_GRENADE);
	AddKeyword("WCL_CLOSERANGE",WCL_CLOSERANGE);
	AddKeyword("PRJ_PIXEL",PRJ_PIXEL);
	AddKeyword("PRJ_IMAGE",PRJ_IMAGE);
	AddKeyword("Bounce",PJ_BOUNCE);
	AddKeyword("Explode",PJ_EXPLODE);
	AddKeyword("Injure",PJ_INJURE);
	AddKeyword("Carve",PJ_CARVE);
	AddKeyword("Dirt",PJ_DIRT);
	AddKeyword("GreenDirt",PJ_GREENDIRT);
	AddKeyword("Disappear",PJ_DISAPPEAR);
	AddKeyword("Nothing",PJ_NOTHING);
	AddKeyword("TRL_NONE",TRL_NONE);
	AddKeyword("TRL_SMOKE",TRL_SMOKE);
	AddKeyword("TRL_CHEMSMOKE",TRL_CHEMSMOKE);
	AddKeyword("TRL_PROJECTILE",TRL_PROJECTILE);
	AddKeyword("TRL_DOOMSDAY",TRL_DOOMSDAY);
	AddKeyword("TRL_EXPLOSIVE",TRL_EXPLOSIVE);
	AddKeyword("SPC_JETPACK",SPC_JETPACK);
	AddKeyword("ANI_ONCE",ANI_ONCE);
	AddKeyword("ANI_LOOP",ANI_LOOP);
	AddKeyword("ANI_PINGPONG",ANI_PINGPONG);
	AddKeyword("true",true);
	AddKeyword("false",false);

	return true;
}




///////////////////
// Compile
int Compile(const char* dir)
{
	char buf[64],wpn[64];
	int num,n;
	sprintf(buf,"%s/Main.txt",dir);


	// Check the file
	FILE *fp = fopen(buf, "rt");
	if(!fp) {
		printf("Error: Could not open the file '%s' for reading\n",buf);
		return false;
	} else
		fclose(fp);


		std::string modname;
		ReadString(buf,"General","ModName", modname,"untitled");
		strcpy(Game->GetWriteableHeader()->ModName, modname.c_str());
	
		printf("Compiling '%s'\n",Game->GetHeader()->ModName);

		ReadInteger(buf,"Weapons","NumWeapons",&num,0);


	// Weapons
		Game->initNewWeapons(num);


	// Compile the weapons
		for(n=0;n<Game->GetNumWeapons();n++) {
			sprintf(wpn,"Weapon%d",n+1);

			std::string weap;
			ReadString(buf,"Weapons",wpn,weap,"");

			if(!CompileWeapon(dir,weap,n))
				return false;
		}

	// Compile the extra stuff
		CompileExtra(dir);

		return true;
}


///////////////////
// Compile a weapon
int CompileWeapon(const char* dir, const std::string& weapon, int id)
{
	weapon_t *Weap = Game->GetWriteableWeapons()+id;
	char file[64];

	sprintf(file,"%s/%s",dir,weapon.c_str());

	Weap->UseSound = false;
	Weap->Special = SPC_NONE;
	Weap->Type = WPN_PROJECTILE;
	Weap->Projectile = NULL;
	Weap->LaserSight = false;

	ReadString(file,"General","Name",Weap->Name,"");
	printf("  Compiling Weapon '%s'\n",Weap->Name.c_str());

	ReadKeyword(file,"General","Type",&Weap->Type,WPN_PROJECTILE);


	// Special Weapons
	if(Weap->Type == WPN_SPECIAL) {
		
		ReadKeyword(file,"General","Special",&Weap->Special,SPC_NONE);

		// If it is a special weapon, read the values depending on the special weapon
		// We don't bother with the 'normal' values
		switch(Weap->Special) {
			// Jetpack
			case SPC_JETPACK:
				CompileJetpack(file, Weap);
				break;

			default:
				printf("   Error: Unknown special type\n");
		}
		return true;
	}


	// Beam Weapons
	if(Weap->Type == WPN_BEAM) {

		CompileBeam(file,Weap);
		return true;
	}


	// Projectile Weapons
	ReadKeyword(file,"General","Class",&Weap->Class,WCL_AUTOMATIC);
	ReadInteger(file,"General","Recoil",&Weap->Recoil,0);
	ReadFloat(file,"General","Recharge",&Weap->Recharge,0);
	ReadFloat(file,"General","Drain",&Weap->Drain,0);
	ReadFloat(file,"General","ROF",&Weap->ROF,0);
	ReadKeyword(file, "General", "LaserSight", &Weap->LaserSight, false);
	if(ReadString(file,"General","Sound",Weap->SndFilename,""))
		Weap->UseSound = true;
	
	
	ReadInteger(file,"Projectile","Speed",&Weap->ProjSpeed,0);
	ReadFloat(file,"Projectile","SpeedVar",&Weap->ProjSpeedVar,0);
	ReadFloat(file,"Projectile","Spread",&Weap->ProjSpread,0);
	ReadInteger(file,"Projectile","Amount",&Weap->ProjAmount,0);


	// Load the projectile
	std::string pfile;
	ReadString(file,"Projectile","Projectile",pfile,"");

	Weap->Projectile = CompileProjectile(dir,pfile.c_str());
	if(Weap->Projectile == NULL)
		return false;

	return true;
}


///////////////////
// Compile a beam weapon
void CompileBeam(const char* file, weapon_t *Weap)
{
	ReadInteger(file,"General","Recoil",&Weap->Recoil,0);
	ReadFloat(file,"General","Recharge",&Weap->Recharge,0);
	ReadFloat(file,"General","Drain",&Weap->Drain,0);
	ReadFloat(file,"General","ROF",&Weap->ROF,0);
	if(ReadString(file,"General","Sound",Weap->SndFilename,""))
		Weap->UseSound = true;

	ReadInteger(file, "Beam", "Damage", &Weap->Bm_Damage, 0);
	ReadInteger(file, "Beam", "Length", &Weap->Bm_Length, 0);
	ReadInteger(file, "Beam", "PlayerDamage", &Weap->Bm_PlyDamage, 0);

	std::string string;
	char *tok;
	ReadString(file,"Beam","Colour",string,"0,0,0");
	char tmp[64];
	strcpy(tmp, string.c_str());
	tok = strtok(tmp,",");	Weap->Bm_Colour[0] = atoi(tok);
	tok = strtok(NULL,",");		Weap->Bm_Colour[1] = atoi(tok);
	tok = strtok(NULL,",");		Weap->Bm_Colour[2] = atoi(tok);
}


///////////////////
// Compile a projectile
proj_t *CompileProjectile(const char* dir, const char* pfile)
{
	proj_t *proj = new proj_t;
	if(proj == NULL)
		return NULL;

	ProjCount++;

	
	// Load the projectile
	char file[128];
	sprintf(file,"%s/%s",dir,pfile);

	printf("    Compiling Projectile '%s'\n",pfile);
	
	proj->filename = pfile;
	
	proj->Timer_Projectiles = false;
	proj->Hit_Projectiles = false;
	proj->PlyHit_Projectiles = false;
	proj->Exp_Projectiles = false;
	proj->Tch_Projectiles = false;
	proj->Projectile = NULL;
	proj->PrjTrl_Proj = NULL;
	proj->Animating = false;
	proj->UseCustomGravity = false;

	ReadKeyword(file,"General","Type",&proj->Type,PRJ_PIXEL);
	ReadFloat(file,"General","Timer",&proj->Timer_Time,0);
	ReadFloat(file, "General", "TimerVar", &proj->Timer_TimeVar, 0);
	ReadKeyword(file,"General","Trail",&proj->Trail,TRL_NONE);
	
	if( ReadInteger(file,"General","Gravity",&proj->Gravity, 0) )
		proj->UseCustomGravity = true;

	ReadFloat(file,"General","Dampening",&proj->Dampening,1.0f);

	if(proj->Type == PRJ_PIXEL) {
		std::string tmp;
		char string[64];
		char *tok;
		proj->NumColours = 1;

		if( ReadString(file,"General","Colour1",tmp,"0,0,0") ) {
			strcpy(string, tmp.c_str());
			tok = strtok(string,",");	proj->Colour1[0] = atoi(tok);
			tok = strtok(NULL,",");		proj->Colour1[1] = atoi(tok);
			tok = strtok(NULL,",");		proj->Colour1[2] = atoi(tok);
		}

		if( ReadString(file,"General","Colour2",tmp,"0,0,0") ) {
			strcpy(string, tmp.c_str());
			tok = strtok(string,",");	proj->Colour2[0] = atoi(tok);
			tok = strtok(NULL,",");		proj->Colour2[1] = atoi(tok);
			tok = strtok(NULL,",");		proj->Colour2[2] = atoi(tok);
			proj->NumColours = 2;
		}

	} else if(proj->Type == PRJ_IMAGE) {
		ReadString(file,"General","Image",proj->ImgFilename,"");
		ReadKeyword(file,"General","Rotating",&proj->Rotating,false);
		ReadInteger(file,"General","RotIncrement",&proj->RotIncrement,0);
		ReadInteger(file,"General","RotSpeed",&proj->RotSpeed,0);
		ReadKeyword(file,"General","UseAngle",&proj->UseAngle,0);
		ReadKeyword(file,"General","UseSpecAngle",&proj->UseSpecAngle,0);
		if(proj->UseAngle || proj->UseSpecAngle)
			ReadInteger(file,"General","AngleImages",&proj->AngleImages,0);

		ReadKeyword(file,"General","Animating",&proj->Animating,0);
		if(proj->Animating) {
			ReadFloat(file,"General","AnimRate",&proj->AnimRate,0);
			ReadKeyword(file,"General","AnimType",&proj->AnimType,ANI_ONCE);
		}
	}
	


	// Hit
	ReadKeyword(file,"Hit","Type",&proj->Hit_Type,PJ_EXPLODE);

	// Hit::Explode
	if(proj->Hit_Type == PJ_EXPLODE) {
		ReadInteger(file,"Hit","Damage",&proj->Hit_Damage,0);
		ReadKeyword(file,"Hit","Projectiles",&proj->Hit_Projectiles,false);
		ReadInteger(file,"Hit","Shake",&proj->Hit_Shake,0);

		proj->Hit_UseSound = false;
		if(ReadString(file,"Hit","Sound",proj->Hit_SndFilename,""))
			proj->Hit_UseSound = true;
	}

	// Hit::Carve
	if(proj->Hit_Type == PJ_CARVE) {
		ReadInteger(file,"Hit","Damage",&proj->Hit_Damage,0);
	}

	// Hit::Bounce
	if(proj->Hit_Type == PJ_BOUNCE) {
		ReadFloat(file,"Hit","BounceCoeff",&proj->Hit_BounceCoeff,0.5);
		ReadInteger(file,"Hit","BounceExplode",&proj->Hit_BounceExplode,0);
	}

	// Timer
	if(proj->Timer_Time > 0) {
		ReadKeyword(file,"Time","Type",&proj->Timer_Type,PJ_EXPLODE);
		if(proj->Timer_Type == PJ_EXPLODE) {
			ReadInteger(file,"Time","Damage",&proj->Timer_Damage,0);
			ReadKeyword(file,"Time","Projectiles",&proj->Timer_Projectiles,false);
			ReadInteger(file,"Time","Shake",&proj->Timer_Shake,0);
		}
	}

	// Player hit
	ReadKeyword(file,"PlayerHit","Type",&proj->PlyHit_Type,PJ_INJURE);

	// PlyHit::Explode || PlyHit::Injure
	if(proj->PlyHit_Type == PJ_EXPLODE || proj->PlyHit_Type == PJ_INJURE) {
		ReadInteger(file,"PlayerHit","Damage",&proj->PlyHit_Damage,0);
		ReadKeyword(file,"PlayerHit","Projectiles",&proj->PlyHit_Projectiles,false);
	}

	// PlyHit::Bounce
	if(proj->PlyHit_Type == PJ_BOUNCE) {
		ReadFloat(file,"PlayerHit","BounceCoeff",&proj->PlyHit_BounceCoeff,0.5);
	}


    // OnExplode
	ReadKeyword( file, "Explode", "Type",       &proj->Exp_Type, PJ_NOTHING );
	ReadInteger( file, "Explode", "Damage",     &proj->Exp_Damage, 0 );
	ReadKeyword( file, "Explode", "Projectiles",&proj->Exp_Projectiles, false );
	ReadInteger( file, "Explode", "Shake",      &proj->Exp_Shake, 0 );
	proj->Exp_UseSound = false;
	if( ReadString(file, "Explode", "Sound", proj->Exp_SndFilename,"") )
		proj->Exp_UseSound = true;


    // Touch
	ReadKeyword( file, "Touch", "Type",       &proj->Tch_Type, PJ_NOTHING );
	ReadInteger( file, "Touch", "Damage",     &proj->Tch_Damage, 0 );
	ReadKeyword( file, "Touch", "Projectiles",&proj->Tch_Projectiles, false );
	ReadInteger( file, "Touch", "Shake",      &proj->Tch_Shake, 0 );
	proj->Tch_UseSound = false;
	if( ReadString(file, "Touch", "Sound", proj->Tch_SndFilename,"") )
		proj->Tch_UseSound = true;


	// Projectiles
	if(proj->Timer_Projectiles || proj->Hit_Projectiles || proj->PlyHit_Projectiles || proj->Exp_Projectiles ||
		  proj->Tch_Projectiles) {
		ReadKeyword(file,"Projectile","Useangle",&proj->ProjUseangle,0);
		ReadInteger(file,"Projectile","Angle",&proj->ProjAngle,0);
		ReadInteger(file,"Projectile","Speed",&proj->ProjSpeed,0);
		ReadFloat(file,"Projectile","SpeedVar",&proj->ProjSpeedVar,0);
		ReadFloat(file,"Projectile","Spread",&proj->ProjSpread,0);
		ReadInteger(file,"Projectile","Amount",&proj->ProjAmount,0);

		// Load the projectile
		std::string prjfile;
		ReadString(file,"Projectile","Projectile",prjfile,"");

		proj->Projectile = CompileProjectile(dir,prjfile.c_str());
		  }


	// Projectile trail
		  if(proj->Trail == TRL_PROJECTILE) {
			  ReadKeyword(file, "ProjectileTrail", "UseProjVelocity", &proj->PrjTrl_UsePrjVelocity, false);
			  ReadFloat  (file, "ProjectileTrail", "Delay",  &proj->PrjTrl_Delay, 100);
			  ReadInteger(file, "ProjectileTrail", "Amount", &proj->PrjTrl_Amount, 1);
			  ReadInteger(file, "ProjectileTrail", "Speed",  &proj->PrjTrl_Speed, 100);
			  ReadFloat(file, "ProjectileTrail", "SpeedVar",  &proj->PrjTrl_SpeedVar, 0);
			  ReadFloat(file, "ProjectileTrail", "Spread", &proj->PrjTrl_Spread, 0);
		
		// Load the projectile
			  std::string prjfile;
			  ReadString(file, "ProjectileTrail", "Projectile", prjfile, "");

			  proj->PrjTrl_Proj = CompileProjectile(dir,prjfile.c_str());
		  }
	

		  return proj;
}


///////////////////
// Compile the extra stuff
int CompileExtra(const char* dir)
{
	char file[64];

	printf("Compiling Extras\n");

	sprintf(file,"%s/main.txt",dir);


	// Ninja Rope
	printf("  Compiling Ninja Rope\n");

	int ropel, restl;
	float strength;

	ReadInteger(file,"NinjaRope","RopeLength",&ropel,0);
	ReadInteger(file,"NinjaRope","RestLength",&restl,0);
	ReadFloat(file,"NinjaRope","Strength",&strength,0);

	Game->SetRopeLength(ropel);
	Game->SetRestLength(restl);
	Game->SetStrength(strength);


	// Worm
	printf("  Compiling Worm\n");
	gs_worm_t *wrm = Game->getWriteableWorm();

	ReadFloat( file, "Worm", "AngleSpeed",		&wrm->AngleSpeed, 150);
	ReadFloat( file, "Worm", "GroundSpeed",		&wrm->GroundSpeed, 8);
	ReadFloat( file, "Worm", "AirSpeed",		&wrm->AirSpeed, 1);
	ReadFloat( file, "Worm", "Gravity",			&wrm->Gravity, 175);
	ReadFloat( file, "Worm", "JumpForce",		&wrm->JumpForce, -140);
	ReadFloat( file, "Worm", "AirFriction",		&wrm->AirFriction, 0);
	ReadFloat( file, "Worm", "GroundFriction",	&wrm->GroundFriction, 0.6f);





	return true;
}


/*
===============================

		Special items

===============================
*/


///////////////////
// Compile the jetpack
int CompileJetpack(const char* file, weapon_t *Weap)
{
	Weap->Projectile = NULL;

	ReadInteger(file, "JetPack", "Thrust", (int*)&Weap->tSpecial.Thrust, 0);
	ReadFloat(file, "JetPack", "Drain", &Weap->Drain, 0);
	ReadFloat(file, "JetPack", "Recharge", &Weap->Recharge, 0);	

	Weap->ROF = 1;

	return true;
}

