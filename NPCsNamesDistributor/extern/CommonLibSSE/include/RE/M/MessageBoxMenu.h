#pragma once

#include "RE/I/IMenu.h"

namespace RE
{
	class MessageBoxData;

	// menuDepth = 10
	// flags = kPausesGame | kAlwaysOpen | kUsesCursor | kModal
	// context = kMenuMode
	class MessageBoxMenu : public IMenu
	{
	public:
		inline static constexpr auto      RTTI = RTTI_MessageBoxMenu;
		inline static constexpr auto      VTABLE = VTABLE_MessageBoxMenu;
		constexpr static std::string_view MENU_NAME = "MessageBoxMenu";

		~MessageBoxMenu() override;  // 00

		// override (IMenu)
		void               Accept(CallbackProcessor* a_processor) override;  // 01
		UI_MESSAGE_RESULTS ProcessMessage(UIMessage& a_message) override;    // 04

		static MessageBoxMenu* GetCurrentMessageBoxMenu();  // inlined in SE
		static std::uint32_t   GetQueueSize();
		static void            QueueMessage(MessageBoxData* a_data);

		// members
		bool          isPopulated;  // 30
		std::uint8_t  pad31;        // 31
		std::uint16_t pad32;        // 32
		std::uint32_t pad34;        // 34
	};
	static_assert(sizeof(MessageBoxMenu) == 0x38);
}
