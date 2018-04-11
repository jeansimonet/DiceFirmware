#include "Settings.h"
#include "Debug.h"
#include "Die.h"

const Settings* settings = (const Settings*)SETTINGS_ADDRESS;

bool Settings::CheckValid() const
{
	return headMarker == SETTINGS_VALID_KEY && tailMarker == SETTINGS_VALID_KEY;
}


ReceiveSettingsSM::ReceiveSettingsSM()
	: currentState(State_Done)
	, FinishedCallbackHandler(nullptr)
	, FinishedCallbackToken(nullptr)
{
}

void ReceiveSettingsSM::Setup(void* token, FinishedCallback handler)
{
	currentState = State_ErasingFlash;
	if (flashPageErase(SETTINGS_PAGE) != 0)
	{
		debugPrintln("Error erasing flash for settings");
		currentState = State_Done;
		return;
	}

	FinishedCallbackHandler = handler;
	FinishedCallbackToken = token;

	// Register for update so we can try to send ack messages
	die.RegisterUpdate(this, [](void* token)
	{
		((ReceiveSettingsSM*)token)->Update();
	});
	currentState = State_SendingAck;
}

void ReceiveSettingsSM::Update()
{
	switch (currentState)
	{
	case State_SendingAck:
		if (die.SendMessage(DieMessage::MessageType_TransferSettingsAck))
		{
			currentState = State_TransferSettings;
			receiveBulkDataSM.Setup();
		}
		// Else try again next frame
		break;
	case State_TransferSettings:
		if (receiveBulkDataSM.TransferComplete())
		{
			// Write the data to flash!
			uint32_t* settingsRaw = (uint32_t*)SETTINGS_ADDRESS;
			flashWrite(settingsRaw, SETTINGS_VALID_KEY);
			settingsRaw += sizeof(uint32_t);
			flashWriteBlock(settingsRaw, receiveBulkDataSM.mallocData, receiveBulkDataSM.mallocSize);
			settingsRaw += receiveBulkDataSM.mallocSize;
			flashWrite(settingsRaw, SETTINGS_VALID_KEY);

			// And we're done!
			receiveBulkDataSM.Finish();
			Finish();
		}
		// Else keep waiting
		break;
	default:
		break;
	}
}

void ReceiveSettingsSM::Finish()
{
	currentState = State_Done;

	if (FinishedCallbackHandler != nullptr)
	{
		FinishedCallbackHandler(FinishedCallbackToken);
		FinishedCallbackHandler = nullptr;
		FinishedCallbackToken = nullptr;
	}
}





SendSettingsSM::SendSettingsSM()
	: currentState(State_Done)
	, FinishedCallbackHandler(nullptr)
	, FinishedCallbackToken(nullptr)
{
}

void SendSettingsSM::Setup(void* token, FinishedCallback handler)
{
	if (settings->CheckValid())
	{
		currentState = State_SendingSetup;

		FinishedCallbackHandler = handler;
		FinishedCallbackToken = token;

		die.RegisterUpdate(this, [](void* token)
		{
			((SendSettingsSM*)token)->Update();
		});
	}
}

void SendSettingsSM::Update()
{
	switch (currentState)
	{
	case State_SendingSetup:
		if (die.SendMessage(DieMessage::MessageType_TransferSettings))
		{
			die.RegisterMessageHandler(DieMessage::MessageType_TransferSettingsAck, this, [](void* token, DieMessage* msg)
			{
				((SendSettingsSM*)token)->currentState = State_SetupAckReceived;
			});

			currentState = State_WaitingForSetupAck;
		}
		// Else try again next frame
		break;
	case SendSettingsSM::State_SetupAckReceived:
		// Unregister ack
		die.UnregisterMessageHandler(DieMessage::MessageType_TransferSettingsAck);

		// Start the bulk transfer
		sendBulkDataSM.Setup((const byte*)&(settings->name), sizeof(Settings) - sizeof(uint32_t) * 2);
		currentState = State_SendingSettings;
		break;
	case SendSettingsSM::State_SendingSettings:
		if (sendBulkDataSM.TransferComplete())
		{
			// We're done!
			sendBulkDataSM.Finish();
			Finish();
		}
		// Else keep waiting
		break;
	default:
		break;
	}
}

void SendSettingsSM::Finish()
{
	currentState = State_Done;

	if (FinishedCallbackHandler != nullptr)
	{
		FinishedCallbackHandler(FinishedCallbackToken);
		FinishedCallbackHandler = nullptr;
		FinishedCallbackToken = nullptr;
	}
}

