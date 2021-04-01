#include "stdafx.h"

#include "media_interface_builder.h"

#include <ds/app/environment.h>
#include <ds/debug/logger.h>
#include <ds/ui/sprite/image.h>
#include <ds/ui/sprite/sprite_engine.h>

#include "ds/ui/media/player/panoramic_video_player.h"
#include "ds/ui/media/player/pdf_player.h"
#include "ds/ui/media/player/stream_player.h"
#include "ds/ui/media/player/video_player.h"
#include "ds/ui/media/player/web_player.h"
#include "ds/ui/media/player/youtube_player.h"

#include "ds/ui/media/interface/pdf_interface.h"
#include "ds/ui/media/interface/video_interface.h"
#include "ds/ui/media/interface/web_interface.h"
#include "ds/ui/media/interface/youtube_interface.h"

namespace ds {
namespace ui {
namespace MediaInterfaceBuilder {

static MediaInterfaceFactoryFunc builderFunc;

MediaInterface* buildMediaInterface(ds::ui::SpriteEngine& engine, ds::ui::Sprite* mediaPlayer, ds::ui::Sprite* parentSprite, const ci::Color buttonColor, const ci::Color backgroundColor) {
	if (builderFunc) {
		return builderFunc(engine, mediaPlayer, parentSprite, buttonColor, backgroundColor);
	}
	else {
		return defaultBuildMediaInterface(engine, mediaPlayer, parentSprite, buttonColor, backgroundColor);
	}
}

MediaInterface* defaultBuildMediaInterface(ds::ui::SpriteEngine& engine, ds::ui::Sprite* mediaPlayer, ds::ui::Sprite* parentSprite,const ci::Color buttonColor, const ci::Color backgroundColor) {
	MediaInterface* outputMi = nullptr;

	if (!parentSprite || !mediaPlayer) {
		DS_LOG_WARNING("Supply parameters to buildMediaInterface, dummy!");
		return outputMi;
	}

	// Images don't have interfaces
	ds::ui::Image* anImage = dynamic_cast<ds::ui::Image*>(mediaPlayer);
	if (anImage) {
		return nullptr;
	}

	ds::ui::VideoPlayer* vidPlayer = dynamic_cast<ds::ui::VideoPlayer*>(mediaPlayer);
	if (vidPlayer) {
		ds::ui::VideoInterface* vi =
			new VideoInterface(engine, ci::vec2(1050.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
		parentSprite->addChildPtr(vi);
		vi->linkVideo(vidPlayer->getVideo());
		outputMi = vi;
	}

	ds::ui::PanoramicVideoPlayer* pvidPlayer = dynamic_cast<ds::ui::PanoramicVideoPlayer*>(mediaPlayer);
	if (pvidPlayer) {
		ds::ui::VideoInterface* vi =
			new VideoInterface(engine, ci::vec2(1050.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
		parentSprite->addChildPtr(vi);
		vi->linkVideo(pvidPlayer->getVideo());
		outputMi = vi;
	}

	ds::ui::StreamPlayer* streamPlayer = dynamic_cast<ds::ui::StreamPlayer*>(mediaPlayer);
	if (streamPlayer) {
		ds::ui::VideoInterface* vi =
			new VideoInterface(engine, ci::vec2(1050.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
		parentSprite->addChildPtr(vi);
		vi->linkVideo(streamPlayer->getVideo());
		outputMi = vi;
	}

	ds::ui::WebPlayer* webPlayer = dynamic_cast<ds::ui::WebPlayer*>(mediaPlayer);
	if (webPlayer) {
		ds::ui::WebInterface* wi = new WebInterface(engine, ci::vec2(400.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
		parentSprite->addChildPtr(wi);
		wi->linkWeb(webPlayer->getWeb());
		outputMi = wi;
	}

	ds::ui::YouTubePlayer* ytPlayer = dynamic_cast<ds::ui::YouTubePlayer*>(mediaPlayer);
	if(ytPlayer){
			ds::ui::YoutubeInterface* wi = new YoutubeInterface(engine, ci::vec2(400.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
			parentSprite->addChildPtr(wi);
			wi->linkYouTubeWeb(ytPlayer->getYouTubeWeb());
			outputMi = wi;		
	}

	ds::ui::PDFPlayer* pdfPlayer = dynamic_cast<ds::ui::PDFPlayer*>(mediaPlayer);
	if (pdfPlayer) {
		ds::ui::PDFInterface* pi =
			new PDFInterface(engine, ci::vec2(400.0f, 50.0f), 25.0f, buttonColor, backgroundColor);
		parentSprite->addChildPtr(pi);
		pi->linkPDF(pdfPlayer->getPDF(), pdfPlayer->getResource());
		outputMi = pi;
	}

	if (!outputMi) {
		DS_LOG_WARNING(
			"This type of media player isn't supported in buildMediaInterface. Support it! Or pass in the correct parameters. Or "
			"don't. I'm a log message, not a cop.");
	}

	return outputMi;
}

void setBuilderFunc(MediaInterfaceFactoryFunc builder) {
	builderFunc = builder;
}

MediaInterfaceFactoryFunc getBuilderFunc() {
	auto temp = builderFunc;
	if (!temp) {
		temp = defaultBuildMediaInterface;
	}
	return temp;
}


}  // namespace MediaInterfaceBuilder
}  // namespace ui
}  // namespace ds
