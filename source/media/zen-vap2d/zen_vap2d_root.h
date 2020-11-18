#pragma once

#include "zen_app.h"
#include "zen_ticker.h"
#include <vector>
#include <mutex>
#include "zen_vap2d_node.h"

namespace Zen { namespace Vap2d {

	class RootDelegate {
	public:
		virtual void onLaunch() {}

		virtual void onResize() {}

		virtual void onExit() {}

		virtual void onPause() {}

		virtual void onResume() {}

		virtual void onUpdate(float interval) {}
		/**
		 @function onBack
		 Android back action
		 */
		virtual void onBack() {}
		/**
		 @function onBack
		 Windows <alt+f4> action
		 */
		virtual void onClose() {}
	};
	
	class Root {

	public:
		static Root * S();

		/**
		 @function run
		 - call this in your MAIN (maybe 'ZenAppMain') function (must & only once).
		 */
		virtual void run(std::shared_ptr<RootDelegate> delegate) = 0;

		virtual std::shared_ptr<RootDelegate> getDelegate() = 0;

	public: // below functions not valid before onLaunch.

		/**
		 @setViewSize
		 - design size
		 */
		virtual void setViewSize(Size2 size) = 0;

		virtual Size2 getViewSize() = 0;

		/**
		 @function getRealViewSize
		 get the application view size.
		 */
		virtual Size2 getRealViewSize() = 0;
		virtual float getViewScale() = 0;
		/**
		 gl clear color
		 */
		virtual void setBackgroundColor(Zen::Color4f color) = 0;
		virtual Zen::Color4f getBackgroundColor() = 0;

		/**
		 total running time ( not includes paused time)
		 */
		virtual std::chrono::microseconds getRunningTime() = 0;

		/**
		 total time since app started (includes paused time).
		 */
		virtual std::chrono::microseconds getTotalTime() = 0;
		/**
		 @function replaceRootNode
		 the original root node will auto remove(). dont remove() or delete any more.
		 */
		virtual void replaceRootNode(Node * node) = 0;
		virtual Node * getRootNode() = 0;

	protected:
		Root() = default;

		virtual ~Root() = default;
	};
}}
