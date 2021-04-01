#include "stdafx.h"

#include "apphost_stats_view.h"

namespace ds {
namespace ui {

AppHostStatsView::AppHostStatsView(ds::ui::SpriteEngine& eng)
	: LayoutSprite(eng)
	, mText(nullptr)
	, mPad(30.0f)
	, mStatus("Unknown")
	, mHttpsRequest(eng)
{
	setShrinkToChildren(ds::ui::LayoutSprite::kShrinkBoth);
	//setSpacing(mPad);

	auto backgroundssy = new ds::ui::Sprite(mEngine);
	backgroundssy->setColor(ci::Color::black());
	backgroundssy->setOpacity(0.8f);
	backgroundssy->mLayoutUserType = ds::ui::LayoutSprite::kFillSize;
	backgroundssy->setTransparent(false);
	addChildPtr(backgroundssy);


	mText = getSomeText();
	if(!mText) {
		DS_LOG_WARNING("Something HORRIBLE HAPPENED!");
		return;
	}
	mText->mLayoutTPad = mPad;
	mText->setFontSize(14.0);

	mHttpsRequest.setVerboseOutput(false);
	mHttpsRequest.setReplyFunction([this](const bool errored, const std::string& reply, long httpCode) {
		if(errored && httpCode == 0) {
			mStatus = "Not running";
			removeButtons();
		} else {
			mStatus = reply;
			addButtons();
		}

		updateText();
	});


	//mEngine.repeatedCallback([this] {updateStats(); }, 5.0);
}

void AppHostStatsView::activate() {

	updateStats();
	show();
}

void AppHostStatsView::deactivate() {
	removeButtons();

	hide();
}

void AppHostStatsView::updateStats() {

	if(!visible()) return;

	mHttpsRequest.makeGetRequest("localhost:7800/api/status");

	updateText();
}

void AppHostStatsView::updateText() {

	if(mText) {
		mText->setText("<span weight='bold'>DSAppHost Status: </span>" + mStatus + "<br>");
	}

	runLayout();
}

void AppHostStatsView::addButton(const std::string& str, const std::string& api, const bool needsConfirm) {
	auto btnText = getSomeText();
	btnText->setText("\t" + str);
	btnText->enable(true);
	btnText->enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);
	btnText->setProcessTouchCallback([this](ds::ui::Sprite* bs, const ds::ui::TouchInfo& ti) {
		if(ti.mPhase == ds::ui::TouchInfo::Added && ti.mNumberFingers == 1) {
			bs->setOpacity(0.5f);
		} else if(ti.mNumberFingers == 0 || glm::distance(ti.mStartPoint, ti.mCurrentGlobalPoint) > mEngine.getMinTapDistance()){
			bs->setOpacity(1.0f);
		}
	});
	if(needsConfirm) {
		setToAskToConfirm(str, api, btnText);
	} else {
		btnText->setTapCallback([this, api](ds::ui::Sprite* bs, const ci::vec3& pos) {
			if(api.empty()) return;
			mHttpsRequest.makeGetRequest("http://localhost:7800/api/" + api);
		});
	}

	mButtons.emplace_back(btnText);
}

void AppHostStatsView::setToConfirm(const std::string str, const std::string api, ds::ui::Text* btnText) {
	if(!btnText) return;

	btnText->setText("Are you sure you want to " + str + "?");
	btnText->setTapCallback([this, api, str, btnText](ds::ui::Sprite* bs, const ci::vec3& pos) {
		if(api.empty()) return;
		mHttpsRequest.makeGetRequest("http://localhost:7800/api/" + api);
		setToAskToConfirm(str, api, btnText);
	});

	btnText->callAfterDelay([this, btnText, str, api] { setToAskToConfirm(str, api, btnText); }, 5.0);
}

void AppHostStatsView::setToAskToConfirm(const std::string str, const std::string api, ds::ui::Text* btnText) {
	if(!btnText) return;
	btnText->cancelDelayedCall();
	btnText->setText("\t" + str);
	btnText->setTapCallback([this, btnText, str, api](ds::ui::Sprite* bs, const ci::vec3& pos) {
		setToConfirm(str, api, btnText);
	});
}

ds::ui::Text* AppHostStatsView::getSomeText() {
	auto btnText = new ds::ui::Text(mEngine);
	btnText->setTextStyle("Arial", 11.0, ci::ColorA::white());
	btnText->setLeading(1.2f);
	btnText->setResizeLimit(400.0f - mPad * 2.0f);
	btnText->mLayoutLPad = mPad;
	btnText->mLayoutRPad = mPad;
	addChildPtr(btnText);
	return btnText;

}

void AppHostStatsView::addButtons() {
	if(mButtons.empty()) {
		addButton("Ping", "ping", false);
		addButton("Status", "status", false);
		addButton("Metadata", "metadata", false);
		addButton("Un-kiosk", "unkiosk", true);
		addButton("Kiosk", "kiosk", true);
		addButton("Start", "start", true);
		addButton("Stop", "stop", true);
		addButton("Reload config", "reconfigure", true);
		addButton("Exit AppHost", "exit", true);
		addButton("Reboot Machine", "reboot", true);
		addButton(" ", "", false); // blank button for spacing
	}
}

void AppHostStatsView::removeButtons() {
	for(auto it : mButtons) {
		it->release();
	}

	mButtons.clear();
}

}
}