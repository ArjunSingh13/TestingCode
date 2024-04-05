//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file  IlluminanceMeasurementServices.h
///   \since  2016-11-11
///   \brief  Definitions to customize server functions for illuminance measurement-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

void IlluminanceMeasurementServer_ActualLightCallback(uint8_t endpoint, uint16_t light);
uint16_t IlluminanceMeasurementServer_GetReverseWinkThreshold(uint8_t endpoint);
EmberAfStatus IlluminanceMeasurementServer_SetLuxCalibrationMultplier(uint8_t endpoint, uint32_t multiplier);
uint32_t IlluminanceMeasurementServer_GetLuxCalibrationMultplier(uint8_t endpoint);
