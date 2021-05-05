/**********************************************************************

Audacity: A Digital Audio Editor

PlayableTrackControls.cpp

Paul Licameli split from TrackInfo.cpp

**********************************************************************/

#include "PlayableTrackControls.h"

#include "../../../Audacity.h"

#include "PlayableTrackButtonHandles.h"
#include "../../../AColor.h"
#include "../../../Track.h"
#include "../../../TrackInfo.h"
#include "../../../TrackPanelDrawingContext.h"
#include "../../../ViewInfo.h"
#include "../../../effects/RealtimeEffectManager.h"

#include <wx/dc.h>

using TCPLine = TrackInfo::TCPLine;

namespace {

void GetNarrowMuteHorizontalBounds( const wxRect & rect, wxRect &dest )
{
   dest.x = rect.x;
   dest.width = rect.width / 2 + 1;
}

void GetNarrowSoloHorizontalBounds( const wxRect & rect, wxRect &dest )
{
   wxRect muteRect;
   GetNarrowMuteHorizontalBounds( rect, muteRect );
   dest.x = rect.x + muteRect.width;
   dest.width = rect.width - muteRect.width + TitleSoloBorderOverlap;
}

void GetEffectsHorizontalBounds( const wxRect & rect, wxRect &dest )
{
   dest.x = rect.x;
   dest.width = rect.width / 2 + 1;
}

void GetBypassHorizontalBounds( const wxRect & rect, wxRect &dest )
{
   wxRect muteRect;
   GetEffectsHorizontalBounds( rect, muteRect );
   dest.x = rect.x + muteRect.width;
   dest.width = rect.width - muteRect.width + TitleSoloBorderOverlap;
}

void GetWideMuteSoloHorizontalBounds( const wxRect & rect, wxRect &dest )
{
   // Larger button, symmetrically placed intended.
   // On windows this gives 15 pixels each side.
   dest.width = rect.width - 2 * kTrackInfoBtnSize + 6;
   dest.x = rect.x + kTrackInfoBtnSize -3;
}

void MuteOrSoloDrawFunction
( wxDC *dc, const wxRect &bev, const Track *pTrack, bool down, 
  bool WXUNUSED(captured),
  bool solo, bool hit )
{
   //bev.Inflate(-1, -1);
   bool selected = pTrack ? pTrack->GetSelected() : true;
   auto pt = dynamic_cast<const PlayableTrack *>(pTrack);
   bool value = pt ? (solo ? pt->GetSolo() : pt->GetMute()) : false;

#if 0
   AColor::MediumTrackInfo( dc, t->GetSelected());
   if( solo )
   {
      if( pt && pt->GetSolo() )
      {
         AColor::Solo(dc, pt->GetSolo(), t->GetSelected());
      }
   }
   else
   {
      if( pt && pt->GetMute() )
      {
         AColor::Mute(dc, pt->GetMute(), t->GetSelected(), pt->GetSolo());
      }
   }
   //(solo) ? AColor::Solo(dc, t->GetSolo(), t->GetSelected()) :
   //    AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
   dc->SetPen( *wxTRANSPARENT_PEN );//No border!
   dc->DrawRectangle(bev);
#endif

   wxCoord textWidth, textHeight;
   wxString str = (solo) ?
      /* i18n-hint: This is on a button that will silence all the other tracks.*/
      _("Solo") :
      /* i18n-hint: This is on a button that will silence this track.*/
      _("Mute");

   AColor::Bevel2(
      *dc,
      value == down,
      bev,
      selected, hit
   );

   TrackInfo::SetTrackInfoFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth) / 2, bev.y + (bev.height - textHeight) / 2);
}

void EffectsOrBypassDrawFunction
( wxDC *dc, const wxRect &bev, const Track *pTrack, bool down, 
  bool WXUNUSED(captured),
  bool bypass, bool hit )
{
   bool selected = true;
   bool value = false;
   
   if (pTrack)
   {
      selected = pTrack->GetSelected();
      if (bypass && pTrack->GetOwner() && pTrack->GetOwner()->GetOwner())
      {
         value = RealtimeEffectManager::Get(*pTrack->GetOwner()->GetOwner()).IsBypassed(*pTrack);
      }
   }

   wxCoord textWidth, textHeight;
   wxString str = (bypass) ?
      _("Bypass") :
      _("Effects");

   AColor::Bevel2(
      *dc,
      value == down,
      bev,
      selected, hit
   );

   TrackInfo::SetTrackInfoFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth) / 2, bev.y + (bev.height - textHeight) / 2);
}

void WideMuteDrawFunction
( TrackPanelDrawingContext &context,
  const wxRect &rect, const Track *pTrack )
{
   auto dc = &context.dc;
   wxRect bev = rect;
   GetWideMuteSoloHorizontalBounds( rect, bev );
   auto target = dynamic_cast<MuteButtonHandle*>( context.target.get() );
   bool hit = target && target->GetTrack().get() == pTrack;
   bool captured = hit && target->IsClicked();
   bool down = captured && bev.Contains( context.lastState.GetPosition());
   MuteOrSoloDrawFunction( dc, bev, pTrack, down, captured, false, hit );
}

void WideSoloDrawFunction
( TrackPanelDrawingContext &context,
  const wxRect &rect, const Track *pTrack )
{
   auto dc = &context.dc;
   wxRect bev = rect;
   GetWideMuteSoloHorizontalBounds( rect, bev );
   auto target = dynamic_cast<SoloButtonHandle*>( context.target.get() );
   bool hit = target && target->GetTrack().get() == pTrack;
   bool captured = hit && target->IsClicked();
   bool down = captured && bev.Contains( context.lastState.GetPosition());
   MuteOrSoloDrawFunction( dc, bev, pTrack, down, captured, true, hit );
}

void MuteAndSoloDrawFunction
( TrackPanelDrawingContext &context,
  const wxRect &rect, const Track *pTrack )
{
   auto dc = &context.dc;
   bool bHasSoloButton = TrackInfo::HasSoloButton();

   wxRect bev = rect;
   if ( bHasSoloButton )
      GetNarrowMuteHorizontalBounds( rect, bev );
   else
      GetWideMuteSoloHorizontalBounds( rect, bev );
   {
      auto target = dynamic_cast<MuteButtonHandle*>( context.target.get() );
      bool hit = target && target->GetTrack().get() == pTrack;
      bool captured = hit && target->IsClicked();
      bool down = captured && bev.Contains( context.lastState.GetPosition());
      MuteOrSoloDrawFunction( dc, bev, pTrack, down, captured, false, hit );
   }

   if( !bHasSoloButton )
      return;

   GetNarrowSoloHorizontalBounds( rect, bev );
   {
      auto target = dynamic_cast<SoloButtonHandle*>( context.target.get() );
      bool hit = target && target->GetTrack().get() == pTrack;
      bool captured = hit && target->IsClicked();
      bool down = captured && bev.Contains( context.lastState.GetPosition());
      MuteOrSoloDrawFunction( dc, bev, pTrack, down, captured, true, hit );
   }
}

void EffectsAndBypassDrawFunction
( TrackPanelDrawingContext &context,
  const wxRect &rect, const Track *pTrack )
{
   auto dc = &context.dc;

   wxRect bev = rect;

   GetEffectsHorizontalBounds( rect, bev );
   {
      auto target = dynamic_cast<EffectsButtonHandle*>( context.target.get() );
      bool hit = target && target->GetTrack().get() == pTrack;
      bool captured = hit && target->IsClicked();
      bool down = captured && bev.Contains( context.lastState.GetPosition());
      EffectsOrBypassDrawFunction( dc, bev, pTrack, down, captured, false, hit );
   }

   GetBypassHorizontalBounds( rect, bev );
   {
      auto target = dynamic_cast<BypassButtonHandle*>( context.target.get() );
      bool hit = target && target->GetTrack().get() == pTrack;
      bool captured = hit && target->IsClicked();
      bool down = captured && bev.Contains( context.lastState.GetPosition());
      EffectsOrBypassDrawFunction( dc, bev, pTrack, down, captured, true, hit );
   }
}
}

void PlayableTrackControls::GetMuteSoloRect
(const wxRect & rect, wxRect & dest, bool solo, bool bHasSoloButton,
 const Track *pTrack)
{
   auto &trackControl = static_cast<const CommonTrackControls&>(
      TrackControls::Get( *pTrack ) );
   auto resultsM = TrackInfo::CalcItemY( trackControl.GetTCPLines(), TCPLine::kItemMute );
   auto resultsS = TrackInfo::CalcItemY( trackControl.GetTCPLines(), TCPLine::kItemSolo );
   dest.height = resultsS.second;

   int yMute = resultsM.first;
   int ySolo = resultsS.first;

   bool bSameRow = ( yMute == ySolo );
   bool bNarrow = bSameRow && bHasSoloButton;

   if( bNarrow )
   {
      if( solo )
         GetNarrowSoloHorizontalBounds( rect, dest );
      else
         GetNarrowMuteHorizontalBounds( rect, dest );
   }
   else
      GetWideMuteSoloHorizontalBounds( rect, dest );

   if( bSameRow || !solo )
      dest.y = rect.y + yMute;
   else
      dest.y = rect.y + ySolo;

}

void PlayableTrackControls::GetEffectsBypassRect
(const wxRect & rect, wxRect & dest, bool bypass, const Track *pTrack)
{
   auto &trackControl = static_cast<const CommonTrackControls&>(
      TrackControls::Get( *pTrack ) );
   auto resultsE = TrackInfo::CalcItemY( trackControl.GetTCPLines(), TCPLine::kItemEffects );
   auto resultsB = TrackInfo::CalcItemY( trackControl.GetTCPLines(), TCPLine::kItemBypass );
   dest.height = resultsB.second;

   int yEffects = resultsE.first;
   int yBypass = resultsB.first;

   bool bSameRow = ( yEffects == yBypass );
   if( bypass )
      GetBypassHorizontalBounds( rect, dest );
   else
      GetEffectsHorizontalBounds( rect, dest );

   dest.y = rect.y + yBypass;

}

#include <mutex>
const TCPLines& PlayableTrackControls::StaticTCPLines()
{
   static TCPLines playableTrackTCPLines;
   static std::once_flag flag;
   std::call_once( flag, []{
      playableTrackTCPLines = CommonTrackControls::StaticTCPLines();
      playableTrackTCPLines.insert( playableTrackTCPLines.end(), {
      { TCPLine::kItemMute | TCPLine::kItemSolo, kTrackInfoBtnSize + 1, 0,
         MuteAndSoloDrawFunction },
      } );
      playableTrackTCPLines.insert( playableTrackTCPLines.end(), {
      { TCPLine::kItemEffects | TCPLine::kItemBypass, kTrackInfoBtnSize + 1, 0,
         EffectsAndBypassDrawFunction },
      } );
   } );
   return playableTrackTCPLines;
}
