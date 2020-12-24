//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_baseanimatingoverlay.h"
#include "bone_setup.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "datacache/imdlcache.h"
#include "eventlist.h"

#include "dt_utlvector_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_sequence_debug;

mstudioevent_t *GetEventIndexForSequence( mstudioseqdesc_t &seqdesc );

C_BaseAnimatingOverlay::C_BaseAnimatingOverlay()
{
	// FIXME: where does this initialization go now?
	//for ( int i=0; i < MAX_OVERLAYS; i++ )
	//{
	//	memset( &m_Layer[i], 0, sizeof(m_Layer[0]) );
	//	m_Layer[i].m_nOrder = MAX_OVERLAYS;
	//}

	// FIXME: where does this initialization go now?
	// AddVar( m_Layer, &m_iv_AnimOverlay, LATCH_ANIMATION_VAR );
}

#undef CBaseAnimatingOverlay

void RecvProxy_SequenceChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CAnimationLayer *pLayer = (CAnimationLayer *)pStruct;

	if ( pLayer->GetOwner() )
		pLayer->GetOwner()->NotifyOnLayerChangeSequence( pLayer, pData->m_Value.m_Int );

	pLayer->SetSequence( pData->m_Value.m_Int );
}

void RecvProxy_WeightChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CAnimationLayer *pLayer = (CAnimationLayer *)pStruct;

	if ( pLayer->GetOwner() )
		pLayer->GetOwner()->NotifyOnLayerChangeWeight( pLayer, pData->m_Value.m_Float );

	pLayer->SetWeight( pData->m_Value.m_Float );
}

void RecvProxy_WeightDeltaRateChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CAnimationLayer *pLayer = (CAnimationLayer *)pStruct;
	pLayer->SetWeightDeltaRate( pData->m_Value.m_Float );
}

void RecvProxy_CycleChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CAnimationLayer *pLayer = (CAnimationLayer *)pStruct;

	if ( pLayer->GetOwner() )
		pLayer->GetOwner()->NotifyOnLayerChangeCycle( pLayer, pData->m_Value.m_Float );

	pLayer->SetCycle( pData->m_Value.m_Float );
}

void RecvProxy_PlaybackRateChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CAnimationLayer *pLayer = (CAnimationLayer *)pStruct;
	pLayer->SetPlaybackRate( pData->m_Value.m_Float );
}

BEGIN_RECV_TABLE_NOBASE(CAnimationLayer, DT_Animationlayer)
	RecvPropInt(	RECVINFO_NAME(m_nSequence, m_nSequence), 0, RecvProxy_SequenceChanged ),
	RecvPropFloat(	RECVINFO_NAME(m_flCycle, m_flCycle), 0, RecvProxy_CycleChanged ),
	RecvPropFloat(	RECVINFO_NAME(m_flPlaybackRate, m_flPlaybackRate), 0, RecvProxy_PlaybackRateChanged ),
	RecvPropFloat(	RECVINFO_NAME(m_flPrevCycle, m_flPrevCycle)),
	RecvPropFloat(	RECVINFO_NAME(m_flWeight, m_flWeight), 0, RecvProxy_WeightChanged ),
	RecvPropFloat(	RECVINFO_NAME(m_flWeightDeltaRate, m_flWeightDeltaRate), 0, RecvProxy_WeightDeltaRateChanged ),
	RecvPropInt(	RECVINFO_NAME(m_nOrder, m_nOrder))
END_RECV_TABLE()

const char *s_m_iv_AnimOverlayNames[C_BaseAnimatingOverlay::MAX_OVERLAYS] =
{
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay00",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay01",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay02",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay03",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay04",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay05",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay06",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay07",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay08",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay09",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay10",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay11",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay12",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay13",
	"C_BaseAnimatingOverlay::m_iv_AnimOverlay14"
};

void ResizeAnimationLayerCallback( void *pStruct, int offsetToUtlVector, int len )
{
	C_BaseAnimatingOverlay *pEnt = (C_BaseAnimatingOverlay*)pStruct;
	CUtlVector < C_AnimationLayer > *pVec = &pEnt->m_AnimOverlay;
	CUtlVector< CInterpolatedVar< C_AnimationLayer > > *pVecIV = &pEnt->m_iv_AnimOverlay;
	
	Assert( (char*)pVec - (char*)pEnt == offsetToUtlVector );
	Assert( pVec->Count() == pVecIV->Count() );
	Assert( pVec->Count() <= C_BaseAnimatingOverlay::MAX_OVERLAYS );
	
	int diff = len - pVec->Count();

	

	if ( diff == 0 )
		return;

	// remove all entries
	for ( int i=0; i < pVec->Count(); i++ )
	{
		pEnt->RemoveVar( &pVec->Element( i ) );
	}

	// adjust vector sizes
	if ( diff > 0 )
	{
		for ( int i = 0; i < diff; ++i )
		{
			int j = pVec->AddToTail();
			(*pVec)[j].SetOwner( pEnt );
		}
		pVec->AddMultipleToTail( diff );
		pVecIV->AddMultipleToTail( diff );
	}
	else
	{
		pVec->RemoveMultiple( len, -diff );
		pVecIV->RemoveMultiple( len, -diff );
	}

	// Rebind all the variables in the ent's list.
	for ( int i=0; i < len; i++ )
	{
		IInterpolatedVar *pWatcher = &pVecIV->Element( i );
		pWatcher->SetDebugName( s_m_iv_AnimOverlayNames[i] );
		pEnt->AddVar( &pVec->Element( i ), pWatcher, LATCH_ANIMATION_VAR, true );
	}
	// FIXME: need to set historical values of nOrder in pVecIV to MAX_OVERLAY
	
}


BEGIN_RECV_TABLE_NOBASE( C_BaseAnimatingOverlay, DT_OverlayVars )
	 RecvPropUtlVector( 
		RECVINFO_UTLVECTOR_SIZEFN( m_AnimOverlay, ResizeAnimationLayerCallback ), 
		C_BaseAnimatingOverlay::MAX_OVERLAYS,
		RecvPropDataTable(NULL, 0, 0, &REFERENCE_RECV_TABLE( DT_Animationlayer ) ) )
END_RECV_TABLE()


IMPLEMENT_CLIENTCLASS_DT( C_BaseAnimatingOverlay, DT_BaseAnimatingOverlay, CBaseAnimatingOverlay )
	RecvPropDataTable( "overlay_vars", 0, 0, &REFERENCE_RECV_TABLE( DT_OverlayVars ) )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseAnimatingOverlay )

/*
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[0][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[1][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[2][2].m_flWeight, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_nSequence, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flCycle, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flPlaybackRate, FIELD_FLOAT),
	DEFINE_FIELD( C_BaseAnimatingOverlay, m_Layer[3][2].m_flWeight, FIELD_FLOAT),
*/

END_PREDICTION_DATA()

void C_AnimationLayer::SetOwner( C_BaseAnimatingOverlay *pOverlay )
{
	m_pOwner = pOverlay;
}

C_BaseAnimatingOverlay *C_AnimationLayer::GetOwner() const
{
	return m_pOwner;
}

C_AnimationLayer* C_BaseAnimatingOverlay::GetAnimOverlay( int i, bool bUseOrder )
{
	Assert( i >= 0 && i < MAX_OVERLAYS );

	if ( !m_AnimOverlay.Count() )
		return NULL;

	if ( bUseOrder )
	{
		FOR_EACH_VEC( m_AnimOverlay, j )
		{
			if ( m_AnimOverlay[j].GetOrder() == i )
				return &m_AnimOverlay[j];
		}
	}

	return &m_AnimOverlay[i];
}


void C_BaseAnimatingOverlay::SetNumAnimOverlays( int num )
{
	if ( m_AnimOverlay.Count() < num )
	{
		int nCountToAdd = num - m_AnimOverlay.Count();
		for ( int i = 0; i < nCountToAdd; ++i )
		{
			int j = m_AnimOverlay.AddToTail();
			m_AnimOverlay[j].SetOwner( this );
		}
	}
	else if ( m_AnimOverlay.Count() > num )
	{
		m_AnimOverlay.RemoveMultiple( num, m_AnimOverlay.Count() - num );
	}

	// Ensure capacity
	m_AnimOverlay.EnsureCapacity( C_BaseAnimatingOverlay::MAX_OVERLAYS );

	int nNumAllocated = m_AnimOverlay.NumAllocated();

	// This is important to do because EnsureCapacity doesn't actually call the constructors
	// on the elements, but we need them to be initialized, otherwise it'll have out-of-range
	// values which will piss off the datatable encoder.
	UtlVector_InitializeAllocatedElements( m_AnimOverlay.Base() + m_AnimOverlay.Count(), nNumAllocated - m_AnimOverlay.Count() );
}


int C_BaseAnimatingOverlay::GetNumAnimOverlays() const
{
	return m_AnimOverlay.Count();
}


void C_BaseAnimatingOverlay::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	BaseClass::GetRenderBounds( theMins, theMaxs );

	if ( !IsRagdoll() )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
			return;

		int nSequences = pStudioHdr->GetNumSeq();

		int i;
		for (i = 0; i < m_AnimOverlay.Count(); i++)
		{
			if ( m_AnimOverlay[i].GetWeight() > 0.0 && m_AnimOverlay[i].GetOrder() != MAX_OVERLAYS )
			{
				if ( m_AnimOverlay[i].GetSequence() >= nSequences )
				{
					continue;
				}

				mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( m_AnimOverlay[i].GetSequence() );
				VectorMin( seqdesc.bbmin, theMins, theMins );
				VectorMax( seqdesc.bbmax, theMaxs, theMaxs );
			}
		}
	}
}



void C_BaseAnimatingOverlay::CheckForLayerChanges( CStudioHdr *hdr, float currentTime )
{
	bool bLayersChanged = false;
	
	// FIXME: damn, there has to be a better way than this.
	int i;
	for (i = 0; i < m_iv_AnimOverlay.Count(); i++)
	{
		CDisableRangeChecks disableRangeChecks; 

		int iHead, iPrev1, iPrev2;
		m_iv_AnimOverlay[i].GetInterpolationInfo( currentTime, &iHead, &iPrev1, &iPrev2 );

		// fake up previous cycle values.
		float t0;
		C_AnimationLayer *pHead = m_iv_AnimOverlay[i].GetHistoryValue( iHead, t0 );
		// reset previous
		float t1;
		C_AnimationLayer *pPrev1 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev1, t1 );
		// reset previous previous
		float t2;
		C_AnimationLayer *pPrev2 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev2, t2 );

		if ( pHead && pPrev1 && pHead->GetSequence() != pPrev1->GetSequence() )
		{
			bLayersChanged = true;
	#if 1 // _DEBUG
			if (/* Q_stristr( hdr->pszName(), r_sequence_debug.GetString()) != NULL || */ r_sequence_debug.GetInt() == entindex())
			{
				DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t0, hdr->pSeqdesc( pHead->GetSequence() ).pszLabel(),  (float)pHead->GetCycle(),  (float)pHead->GetWeight(), i );
				DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t1, hdr->pSeqdesc( pPrev1->GetSequence() ).pszLabel(),  (float)pPrev1->GetCycle(), (float)pPrev1->GetWeight(), i );
				if (pPrev2)
					DevMsgRT( "(%7.4f : %30s : %5.3f : %4.2f : %1d)\n", t2, hdr->pSeqdesc( pPrev2->GetSequence() ).pszLabel(),  (float)pPrev2->GetCycle(),  (float)pPrev2->GetWeight(), i );
			}
	#endif

			if (pPrev1)
			{
				pPrev1->SetSequence( pHead->GetSequence() );
				pPrev1->SetCycle( pHead->GetPrevCycle() );
				pPrev1->SetWeight( pHead->GetWeight() );
			}

			if (pPrev2)
			{
				float num = 0;
				if ( fabs( t0 - t1 ) > 0.001f )
					num = (t2 - t1) / (t0 - t1);

				pPrev2->SetSequence( pHead->GetSequence() );
				float flTemp;
				if (IsSequenceLooping( hdr, pHead->GetSequence() ))
				{
					flTemp = LoopingLerp( num, (float)pHead->GetPrevCycle(), (float)pHead->GetCycle() );
				}
				else
				{
					flTemp = Lerp( num, (float)pHead->GetPrevCycle(), (float)pHead->GetCycle() );
				}
				pPrev2->SetCycle( flTemp );
				pPrev2->SetWeight( pHead->GetWeight() );
			}

			/*
			if (stricmp( r_seq_overlay_debug.GetString(), hdr->name ) == 0)
			{
				DevMsgRT( "(%30s %6.2f : %6.2f : %6.2f)\n", hdr->pSeqdesc( pHead->nSequence ).pszLabel(), (float)pPrev2->m_flCycle, (float)pPrev1->m_flCycle, (float)pHead->m_flCycle );
			}
			*/

			m_iv_AnimOverlay[i].SetLooping( IsSequenceLooping( hdr, pHead->GetSequence() ) );
			m_iv_AnimOverlay[i].Interpolate( currentTime );

			// reset event indexes
			m_flOverlayPrevEventCycle[i] = pHead->GetPrevCycle() - 0.01;
		}
	}

	if (bLayersChanged)
	{
		// render bounds may have changed
		UpdateVisibility();
	}
}



void C_BaseAnimatingOverlay::AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	BaseClass::AccumulateLayers( boneSetup, pos, q, currentTime );
	int i;

	// resort the layers
	int layer[MAX_OVERLAYS];
	for (i = 0; i < MAX_OVERLAYS; i++)
	{
		layer[i] = MAX_OVERLAYS;
	}

	for (i = 0; i < m_AnimOverlay.Count(); i++)
	{
		CAnimationLayer *pLayer = GetAnimOverlay( i );
		if ( pLayer )
		{
			layer[i] = clamp( pLayer->GetOrder(), 0, MAX_OVERLAYS - 1 );
		}
	}

	CheckForLayerChanges( boneSetup.GetStudioHdr(), currentTime );

	int nSequences = boneSetup.GetStudioHdr()->GetNumSeq();

	// add in the overlay layers
	int j;
	for (j = 0; j < MAX_OVERLAYS; j++)
	{
		i = layer[ j ];
		if ( i >= m_AnimOverlay.Count() )
		{
#if defined( DEBUG_TF2_OVERLAYS )
			engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
#endif
			continue;
		}

		if ( m_AnimOverlay[i].GetSequence() >= nSequences )
			continue;

		/*
		DevMsgRT( 1 , "%.3f  %.3f  %.3f\n", currentTime, fWeight, dadt );
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -j - 1, 0, 
			"%2d(%s) : %6.2f : %6.2f", 
				m_AnimOverlay[i].m_nSequence,
				boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence )->pszLabel(),
				m_AnimOverlay[i].m_flCycle, 
				m_AnimOverlay[i].m_flWeight
				);
		*/

		float fWeight = m_AnimOverlay[i].GetWeight();
		if ( fWeight <= 0.0f )
		{
#if defined( DEBUG_TF2_OVERLAYS )
			engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", "            ", 0.f, 0.f, i );
#endif
			continue;
		}

		// check to see if the sequence changed
		// FIXME: move this to somewhere more reasonable
		// do a nice spline interpolation of the values
		// if ( m_AnimOverlay[i].m_nSequence != m_iv_AnimOverlay.GetPrev( i )->nSequence )
		float fCycle = m_AnimOverlay[ i ].GetCycle();
		fCycle = ClampCycle( fCycle, IsSequenceLooping( m_AnimOverlay[i].GetSequence() ) );

		if ( !IsFinite( fCycle ) )
		{
			AssertMsg( false, "fCycle is nan!" );
			fCycle = 0;
		}

		if (fWeight > 1.0f)
		{
			fWeight = 1.0f;
		}

		boneSetup.AccumulatePose( pos, q, m_AnimOverlay[i].GetSequence(), fCycle, fWeight, currentTime, m_pIk );

#if defined( DEBUG_TF2_OVERLAYS )
		engine->Con_NPrintf( 10 + j, "%30s %6.2f : %6.2f : %1d", boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].m_nSequence ).pszLabel(), fCycle, fWeight, i );
#endif

#if 1 // _DEBUG
		if (r_sequence_debug.GetInt() == entindex())
		{
			if (1)
			{
				DevMsgRT( "%8.4f : %30s : %5.3f : %4.2f : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].GetSequence() ).pszLabel(), fCycle, fWeight, i );
			}
			else
			{
				int iHead, iPrev1, iPrev2;
				m_iv_AnimOverlay[i].GetInterpolationInfo( currentTime, &iHead, &iPrev1, &iPrev2 );

				// fake up previous cycle values.
				float t0;
				CAnimationLayer *pHead = m_iv_AnimOverlay[i].GetHistoryValue( iHead, t0 );
				// reset previous
				float t1;
				CAnimationLayer *pPrev1 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev1, t1 );
				// reset previous previous
				float t2;
				CAnimationLayer *pPrev2 = m_iv_AnimOverlay[i].GetHistoryValue( iPrev2, t2 );

				if ( pHead && pPrev1 && pPrev2 )
				{
					DevMsgRT( "%6.2f : %30s %6.2f (%6.2f:%6.2f:%6.2f) : %6.2f (%6.2f:%6.2f:%6.2f) : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].GetSequence() ).pszLabel(), 
						fCycle, (float)pPrev2->GetCycle(), (float)pPrev1->GetCycle(), (float)pHead->GetCycle(),
						fWeight, (float)pPrev2->GetWeight(), (float)pPrev1->GetWeight(), (float)pHead->GetWeight(),
						i );
				}
				else
				{
					DevMsgRT( "%6.2f : %30s %6.2f : %6.2f : %1d\n", currentTime, boneSetup.GetStudioHdr()->pSeqdesc( m_AnimOverlay[i].GetSequence() ).pszLabel(), fCycle, fWeight, i );
				}

			}
		}
#endif
	}
	//RegenerateDispatchedLayers( boneSetup, pos, q, currentTime );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the sequence or weapon changed, if so find a matching sequence or clear the dispatch
//-----------------------------------------------------------------------------

bool C_BaseAnimatingOverlay::UpdateDispatchLayer( CAnimationLayer *pLayer, CStudioHdr *pWeaponStudioHdr, int iSequence )
{
	if ( !pWeaponStudioHdr || !pLayer )
	{
		if ( pLayer )
			pLayer->m_nDispatchedDst = ACT_INVALID;
		return false;
	}	

	if ( pLayer->m_pDispatchedStudioHdr != pWeaponStudioHdr || pLayer->m_nDispatchedSrc != iSequence || pLayer->m_nDispatchedDst >= pWeaponStudioHdr->GetNumSeq()  )
	{
		pLayer->m_pDispatchedStudioHdr = pWeaponStudioHdr;
		pLayer->m_nDispatchedSrc = iSequence;
		if ( pWeaponStudioHdr )
		{
			const char *pszLayerName = GetSequenceName( iSequence );
			pLayer->m_nDispatchedDst = pWeaponStudioHdr->LookupSequence( pszLayerName );
		}
		else
		{
			pLayer->m_nDispatchedDst = ACT_INVALID;
		}
	}
	return (pLayer->m_nDispatchedDst != ACT_INVALID );
}


//-----------------------------------------------------------------------------
// Purpose: Play overlay sequences on dispatched model, merge results back to parent model
//-----------------------------------------------------------------------------

void C_BaseAnimatingOverlay::AccumulateInterleavedDispatchedLayers( C_BaseAnimatingOverlay *pWeapon, IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime, bool bSetupInvisibleWeapon /* = false */ )
{
	bool bSetupWeapon = pWeapon != NULL && pWeapon->m_pBoneMergeCache != NULL && (pWeapon->IsVisible() || bSetupInvisibleWeapon) ;
	
	// reset event frame indices, etc
	CheckForLayerChanges( boneSetup.GetStudioHdr(), currentTime );

	if ( bSetupWeapon )
	{
		CStudioHdr *pWeaponStudioHdr = pWeapon->GetModelPtr();

		// copy matching player pose params to weapon pose params
		pWeapon->m_pBoneMergeCache->MergeMatchingPoseParams();
		float poseparam[MAXSTUDIOPOSEPARAM];
		pWeapon->GetPoseParameters( pWeaponStudioHdr, poseparam );

		// build a temporary setup for the weapon
		CIKContext weaponIK;
		weaponIK.Init( pWeaponStudioHdr, GetAbsAngles(), GetAbsOrigin(), gpGlobals->curtime, 0, BONE_USED_BY_BONE_MERGE );

		IBoneSetup weaponSetup( pWeaponStudioHdr, BONE_USED_BY_BONE_MERGE, poseparam );
		Vector weaponPos[MAXSTUDIOBONES];
		QuaternionAligned weaponQ[MAXSTUDIOBONES];

		int nSequences = boneSetup.GetStudioHdr()->GetNumSeq();
		for ( int nLayerIdx = 0; nLayerIdx < GetNumAnimOverlays(); nLayerIdx++ )
		{
			CAnimationLayer *pLayer = GetAnimOverlay(nLayerIdx);

			if ( pLayer->GetSequence() <= 1 || pLayer->GetSequence() >= nSequences || pLayer->GetWeight() <= 0 )
				continue;

			float fCycle = pLayer->GetCycle();
			fCycle = ClampCycle( fCycle, IsSequenceLooping( pLayer->GetSequence() ) );

			UpdateDispatchLayer( pLayer, pWeaponStudioHdr, pLayer->GetSequence() );

			if ( pLayer->m_nDispatchedDst > 0 && pLayer->m_nDispatchedDst < pWeaponStudioHdr->GetNumSeq() )
			{
				// copy player bones to weapon setup bones
				pWeapon->m_pBoneMergeCache->CopyFromFollow( pos, q, BONE_USED_BY_BONE_MERGE, weaponPos, weaponQ );

				// respect ik rules on archetypal sequence, even if we're not playing it
				//mstudioseqdesc_t &seqdesc = ((CStudioHdr *)m_pStudioHdr)->pSeqdesc( pLayer->GetSequence() );
				//m_pIk->AddDependencies( seqdesc, pLayer->GetSequence(), pLayer->GetCycle(), m_flPoseParameter, pLayer->GetWeight() );

				// now that the weapon bones are in the position of the current player bones, set up the weapon animation onto that
				weaponSetup.AccumulatePose( weaponPos, weaponQ, pLayer->m_nDispatchedDst, pLayer->GetCycle(), pLayer->GetWeight(), currentTime, &weaponIK );

				//DrawSkeleton( this->GetModelPtr(), BONE_USED_BY_ANYTHING );
				//pWeapon->DrawSkeleton( pWeaponStudioHdr, BONE_USED_BY_ANYTHING );

				// merge weapon bones back
				pWeapon->m_pBoneMergeCache->CopyToFollow( weaponPos, weaponQ, BONE_USED_BY_BONE_MERGE, pos, q );

				weaponIK.CopyTo( m_pIk, pWeapon->m_pBoneMergeCache->GetRawIndexMapping() );
			}
			else
			{
				boneSetup.AccumulatePose( pos, q, pLayer->GetSequence(), fCycle, pLayer->GetWeight(), currentTime, m_pIk );
			}

		}

		//if ( bRanAnyWeaponLayers )
		//{
		//	
		//
		//	CBoneBitList boneComputed;
		//	
		//	pWeapon->UpdateIKLocks( currentTime );
		//	weaponIK.UpdateTargets( pos, q, pWeapon->m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );
		//	
		//	pWeapon->CalculateIKLocks( currentTime );
		//	weaponIK.SolveDependencies( pos, q, pWeapon->m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );
		//	
		//}
	}
	else
	{
		int nSequences = boneSetup.GetStudioHdr()->GetNumSeq();
		for ( int nLayerIdx = 0; nLayerIdx < GetNumAnimOverlays(); nLayerIdx++ )
		{
			CAnimationLayer *pLayer = GetAnimOverlay(nLayerIdx);

			if ( pLayer->GetSequence() < 0 || pLayer->GetSequence() >= nSequences || pLayer->GetWeight() <= 0 )
				continue;

			float fCycle = pLayer->GetCycle();
			fCycle = ClampCycle( fCycle, IsSequenceLooping( pLayer->GetSequence() ) );

			boneSetup.AccumulatePose( pos, q, pLayer->GetSequence(), fCycle, pLayer->GetWeight(), currentTime, m_pIk );
		}
	}

}


void C_BaseAnimatingOverlay::AccumulateDispatchedLayers( C_BaseAnimatingOverlay *pWeapon, CStudioHdr *pWeaponStudioHdr,  IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	if ( !pWeapon->m_pBoneMergeCache )
		return;

	if ( !pWeapon->IsVisible() )
		return;

	// copy matching player pose params to weapon pose params
	pWeapon->m_pBoneMergeCache->MergeMatchingPoseParams();
	float		poseparam[MAXSTUDIOPOSEPARAM];
	pWeapon->GetPoseParameters( pWeaponStudioHdr, poseparam );

	// build a temporary setup for the weapon
	CIKContext weaponIK;
	weaponIK.Init( pWeaponStudioHdr, GetAbsAngles(), GetAbsOrigin(), gpGlobals->curtime, 0, BONE_USED_BY_BONE_MERGE );

	IBoneSetup weaponSetup( pWeaponStudioHdr, BONE_USED_BY_BONE_MERGE, poseparam );
	Vector weaponPos[MAXSTUDIOBONES];
	QuaternionAligned weaponQ[MAXSTUDIOBONES];

	// copy player bones to weapon setup bones
	pWeapon->m_pBoneMergeCache->CopyFromFollow( pos, q, BONE_USED_BY_BONE_MERGE, weaponPos, weaponQ );

	// do layer animations
	// FIXME: some of the layers are player layers, not weapon layers
	// FIXME: how to interleave?
	for ( int i=0; i < GetNumAnimOverlays(); i++ )
	{
		CAnimationLayer *pLayer = GetAnimOverlay( i );
		if ( pLayer->GetOrder() >= MAX_OVERLAYS || pLayer->GetSequence() <= 1 || pLayer->GetWeight() <= 0.0f )
			continue;

		UpdateDispatchLayer( pLayer, pWeaponStudioHdr, pLayer->GetSequence() );

		if ( pLayer->m_nDispatchedDst > 0 && pLayer->m_nDispatchedDst < pWeaponStudioHdr->GetNumSeq() )
		{
			weaponSetup.AccumulatePose( weaponPos, weaponQ, pLayer->m_nDispatchedDst, pLayer->GetCycle(), pLayer->GetWeight(), currentTime, &weaponIK );
		}
	}
	// FIXME: merge weaponIK into m_pIK

	CBoneBitList boneComputed;
	
	pWeapon->UpdateIKLocks( currentTime );
	weaponIK.UpdateTargets( weaponPos, weaponQ, pWeapon->m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );

	pWeapon->CalculateIKLocks( currentTime );
	weaponIK.SolveDependencies( weaponPos, weaponQ, pWeapon->m_BoneAccessor.GetBoneArrayForWrite(), boneComputed );

	// merge weapon bones back
	pWeapon->m_pBoneMergeCache->CopyToFollow( weaponPos, weaponQ, BONE_USED_BY_BONE_MERGE, pos, q );
}

//-----------------------------------------------------------------------------
// Purpose: Duplicate parent models dispatched overlay sequences so that any local bones get animated
//-----------------------------------------------------------------------------
	
void C_BaseAnimatingOverlay::RegenerateDispatchedLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime )
{
	// find who I'm following and see if I'm their dispatched model
	if ( m_pBoneMergeCache && m_pBoneMergeCache->IsCopied() )
	{
		C_BaseEntity *pFollowEnt = GetFollowedEntity();
		if ( pFollowEnt )
		{
			C_BaseAnimatingOverlay *pFollow = pFollowEnt->GetBaseAnimatingOverlay();
			if ( pFollow )
			{
				for ( int i=0; i < pFollow->GetNumAnimOverlays(); i++ )
				{
					CAnimationLayer *pLayer = pFollow->GetAnimOverlay( i );
					if ( pLayer->m_pDispatchedStudioHdr == NULL || pLayer->GetOrder() >= MAX_OVERLAYS || pLayer->GetSequence() == -1 || pLayer->GetWeight() <= 0.0f )
						continue;

					// FIXME: why do the CStudioHdr's not match?
					if ( pLayer->m_pDispatchedStudioHdr->GetRenderHdr() == boneSetup.GetStudioHdr()->GetRenderHdr() )
					{
						if ( pLayer->m_nDispatchedDst != ACT_INVALID )
						{
							boneSetup.AccumulatePose( pos, q, pLayer->m_nDispatchedDst, pLayer->GetCycle(), pLayer->GetWeight(), currentTime, m_pIk );
						}
					}
				}
			}
		}
	}
}

void C_BaseAnimatingOverlay::DoAnimationEvents( CStudioHdr *pStudioHdr )
{
	MDLCACHE_CRITICAL_SECTION();
	if ( !pStudioHdr || !pStudioHdr->SequencesAvailable() )
		return;

	int nSequences = pStudioHdr->GetNumSeq();

	BaseClass::DoAnimationEvents( pStudioHdr );

	bool watch = false; // Q_strstr( hdr->name, "rifle" ) ? true : false;

	CheckForLayerChanges( pStudioHdr, gpGlobals->curtime ); // !!!

	int j;
	for (j = 0; j < m_AnimOverlay.Count(); j++)
	{
		if ( m_AnimOverlay[j].GetSequence() < 0 || m_AnimOverlay[j].GetSequence() >= nSequences )
		{
			continue;
		}

		// Don't bother with 0-weight layers
		if ( m_AnimOverlay[j].GetWeight() == 0.0f || m_AnimOverlay[j].GetOrder() == MAX_OVERLAYS )
		{
			continue;
		}

		mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( m_AnimOverlay[j].GetSequence() );
		if ( seqdesc.numevents == 0 )
			continue;

		// stalled?
		if (m_AnimOverlay[j].GetCycle() == m_flOverlayPrevEventCycle[j])
			continue;

		bool bLoopingSequence = IsSequenceLooping( m_AnimOverlay[j].GetSequence() );

		bool bLooped = false;

		//in client code, m_flOverlayPrevEventCycle is set to -1 when we first start an overlay, looping or not
		if ( bLoopingSequence &&
			m_flOverlayPrevEventCycle[j] > 0.0f &&
			m_AnimOverlay[j].GetCycle() <= m_flOverlayPrevEventCycle[j] )
		{
			if (m_flOverlayPrevEventCycle[j] - m_AnimOverlay[j].GetCycle() > 0.5)
			{
				bLooped = true;
			}
			else
			{
				// things have backed up, which is bad since it'll probably result in a hitch in the animation playback
				// but, don't play events again for the same time slice
				return;
			}
		}

		mstudioevent_t *pevent = GetEventIndexForSequence( seqdesc );

		// This makes sure events that occur at the end of a sequence occur are
		// sent before events that occur at the beginning of a sequence.
		if (bLooped)
		{
			for (int i = 0; i < (int)seqdesc.numevents; i++)
			{
				// ignore all non-client-side events
				if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
				{
					if ( !(pevent[i].type & AE_TYPE_CLIENT) )
						 continue;
				}
				else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
					continue;
			
				if ( pevent[i].cycle <= m_flOverlayPrevEventCycle[j] )
					continue;
				
				if ( watch )
				{
					Msg( "%i FE %i Looped cycle %f, prev %f ev %f (time %.3f)\n",
						gpGlobals->tickcount,
						pevent[i].event,
						pevent[i].cycle,
						(float)m_flOverlayPrevEventCycle[j],
						(float)m_AnimOverlay[j].GetCycle(),
						gpGlobals->curtime );
				}
					
					
				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
			}

			// Necessary to get the next loop working
			m_flOverlayPrevEventCycle[j] = -0.01;
		}

		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
			{
				if ( !(pevent[i].type & AE_TYPE_CLIENT) )
					 continue;
			}
			else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
				continue;

			bool bStartedSequence = ( m_flOverlayPrevEventCycle[j] > m_AnimOverlay[j].GetCycle() || m_flOverlayPrevEventCycle[j] == 0 );

			if ( ( ( pevent[i].cycle > m_flOverlayPrevEventCycle[j] || bStartedSequence && pevent[i].cycle == 0 ) && pevent[i].cycle <= m_AnimOverlay[j].GetCycle()) )
			{
				if ( watch )
				{
					Msg( "%i (seq: %d) FE %i Normal cycle %f, prev %f ev %f (time %.3f)\n",
						gpGlobals->tickcount,
						(int)m_AnimOverlay[j].GetSequence(),
						(int)pevent[i].event,
						(float)pevent[i].cycle,
						(float)m_flOverlayPrevEventCycle[j],
						(float)m_AnimOverlay[j].GetCycle(),
						gpGlobals->curtime );
				}

				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
			}
		}

		m_flOverlayPrevEventCycle[j] = m_AnimOverlay[j].GetCycle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_BaseAnimatingOverlay::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	//// Clear out animation layers
	//for ( int i=0; i < m_AnimOverlay.Count(); i++ )
	//{
	//	m_AnimOverlay[i].Reset();
	//	m_AnimOverlay[i].m_nOrder = MAX_OVERLAYS;
	//}

	return hdr;
}
