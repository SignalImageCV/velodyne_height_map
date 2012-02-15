/**
 * \file  CalibrationUtilities.cc
 * \brief  
 *
 * \author  Piyush Khandelwal (piyushk@cs.utexas.edu)
 * Copyright (C) 2012, Austin Robot Technology, The University of Texas at Austin
 *
 * License: Modified BSD License
 *
 * $ Id: 02/14/2012 11:36:36 AM piyushk $
 */

#include <iostream>
#include <fstream>
#include <string>
#include <velodyne_calibration/CalibrationUtilities.h>
#include <yaml-cpp/yaml.h>

namespace velodyne {

  const std::string NUM_LASERS = "num_lasers";
  const std::string PITCH = "pitch";
  const std::string ROLL = "roll";
  const std::string LASERS = "lasers";
  const std::string LASER_ID = "laser_id";
  const std::string ROT_CORRECTION = "rot_correction";
  const std::string VERT_CORRECTION = "vert_correction";
  const std::string DIST_CORRECTION = "dist_correction";
  const std::string VERT_OFFSET_CORRECTION = "vert_offset_correction";
  const std::string HORIZ_OFFSET_CORRECTION = "vert_offset_correction";
  const std::string INTENSITY = "intensity";
  const std::string A = "a";
  const std::string B = "b";
  const std::string C = "c";
  const std::string D = "d";

  void operator >> (const YAML::Node& node, IntensityCorrection& intensity_correction) {
    node[A] >> intensity_correction.a;
    node[B] >> intensity_correction.b;
    node[C] >> intensity_correction.c;
    node[D] >> intensity_correction.d;
  }

  void operator >> (const YAML::Node& node, std::pair<int, LaserCorrection>& correction) {
    node[LASER_ID] >> correction.first;
    node[ROT_CORRECTION] >> correction.second.rot_correction;
    node[VERT_CORRECTION] >> correction.second.vert_correction;
    node[DIST_CORRECTION] >> correction.second.dist_correction;
    node[VERT_OFFSET_CORRECTION] >> correction.second.vert_offset_correction;
    node[HORIZ_OFFSET_CORRECTION] >> correction.second.horiz_offset_correction;
    const YAML::Node& intensity = node[INTENSITY];
    intensity[0] >> correction.second.intensity_correction;
  }

  void operator >> (const YAML::Node& node, Calibration& calibration) {
    node[NUM_LASERS] >> calibration.num_lasers;
    node[PITCH] >> calibration.pitch;
    node[ROLL] >> calibration.roll;

    const YAML::Node& lasers = node[LASERS];
    calibration.laser_corrections.clear();
    for (int i = 0; i < calibration.num_lasers; i++) {
      std::pair<int, LaserCorrection> correction;
      lasers[i] >> correction;
      calibration.laser_corrections.insert(correction);     
    }
  }

  YAML::Emitter& operator << (YAML::Emitter& out, const IntensityCorrection& intensity_correction) {
    out << YAML::BeginMap;
    out << YAML::Key << A << YAML::Value << intensity_correction.a;
    out << YAML::Key << B << YAML::Value << intensity_correction.b;
    out << YAML::Key << C << YAML::Value << intensity_correction.c;
    out << YAML::Key << D << YAML::Value << intensity_correction.d;
    out << YAML::EndMap;
    return out;
  }

  YAML::Emitter& operator << (YAML::Emitter& out, const std::pair<int, LaserCorrection> correction) {
    out << YAML::BeginMap;
    out << YAML::Key << LASER_ID << YAML::Value << correction.first;
    out << YAML::Key << ROT_CORRECTION << YAML::Value << correction.second.rot_correction;
    out << YAML::Key << VERT_CORRECTION << YAML::Value << correction.second.vert_correction;
    out << YAML::Key << DIST_CORRECTION << YAML::Value << correction.second.dist_correction;
    out << YAML::Key << VERT_OFFSET_CORRECTION << YAML::Value << correction.second.vert_offset_correction;
    out << YAML::Key << HORIZ_OFFSET_CORRECTION << YAML::Value << correction.second.horiz_offset_correction;
    out << YAML::Key << INTENSITY << YAML::Value << YAML::BeginSeq << correction.second.intensity_correction << YAML::EndSeq;
    out << YAML::EndMap;
    return out;
  }

  YAML::Emitter& operator << (YAML::Emitter& out, const Calibration& calibration) {
    out << YAML::BeginMap;
    out << YAML::Key << NUM_LASERS << YAML::Value << calibration.num_lasers;
    out << YAML::Key << PITCH << YAML::Value << calibration.pitch;
    out << YAML::Key << ROLL << YAML::Value << calibration.roll;
    out << YAML::Key << LASERS << YAML::Value << YAML::BeginSeq;
    for (std::map<int, LaserCorrection>::const_iterator it = calibration.laser_corrections.begin();
         it != calibration.laser_corrections.end(); it++) {
      out << *it; 
      //out << YAML::BeginMap << YAML::Key << NUM_LASERS << YAML::Value << calibration.num_lasers << YAML::EndMap;
      //out << "test";
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return out;
  }

  void readCalibrationFromFile(const std::string& calibration_file, 
      Calibration& calibration) {
    std::ifstream fin(calibration_file.c_str());
    YAML::Parser parser(fin);
    YAML::Node doc;
    parser.GetNextDocument(doc);
    doc >> calibration;
    fin.close();
  }

  void writeCalibrationToFile(const std::string& calibration_file, 
      const Calibration& calibration) {
    std::ofstream fout(calibration_file.c_str());
    YAML::Emitter out;
    out << calibration;
    fout << out.c_str();
    fout.close();
  }

  void writeTestCalibrationFile(const std::string& test_calibration_file) {

    Calibration test_calibration;
    test_calibration.num_lasers=64;
    test_calibration.pitch = test_calibration.roll = 0;
    for (int i = 0; i < test_calibration.num_lasers; i++) {
      std::pair<int, LaserCorrection> correction(i, LaserCorrection());
      test_calibration.laser_corrections.insert(correction);
    }

    writeCalibrationToFile(test_calibration_file, test_calibration);

  }

  /** \brief convert raw packet to laserscan format */
  void packet2scans(const raw_packet_t *raw, std::vector<laserscan_t>& scans) {
    int index = 0;                      // current scans entry
    uint16_t revolution = raw->revolution; // current revolution (mod 65536)

    for (int i = 0; i < BLOCKS_PER_PACKET; i++) {
        int bank_origin = 32;
        correction_angles *corrections = upper_;
        if (raw->blocks[i].header == LOWER_BANK)
          {
            bank_origin = 0;
            corrections = lower_;
          }

        float rotation = angles::from_degrees(raw->blocks[i].rotation
                                              * ROTATION_RESOLUTION);

        for (int j = 0, k = 0; j < SCANS_PER_BLOCK; j++, k += RAW_SCAN_SIZE)
          {
            scans[index].laser_number = j + bank_origin;

            //if(!corrections[j].enabled) 
            //  do what???

            // beware: the Velodyne turns clockwise
            scans[index].heading = 
              angles::normalize_angle(-(rotation - corrections[j].rotational));
            scans[index].pitch   = corrections[j].vertical;
      
            union two_bytes tmp;
            tmp.bytes[0] = raw->blocks[i].data[k];
            tmp.bytes[1] = raw->blocks[i].data[k+1];

            // convert range to meters and apply quadratic correction
            scans[index].range = tmp.uint * DISTANCE_RESOLUTION;
            scans[index].range =
              (corrections[j].offset1 * scans[index].range * scans[index].range
               + corrections[j].offset2 * scans[index].range
               + corrections[j].offset3);
      
            scans[index].intensity = raw->blocks[i].data[k+2];
            scans[index].revolution = revolution;

            ++index;
          }
      }

    ROS_ASSERT(index == SCANS_PER_PACKET);
  }
  /** \brief Process Velodyne packet. */
  void processPacket(const velodyne_msgs::VelodynePacket *pkt) {
    // unpack scans from the raw packet
    std::vector<laserscan_t> scans;
    scans.resize(SCANS_PER_PACKET);
    packet2scans((raw_packet_t *) &pkt->data[0], scans);
  }

  template <typename PointT>
  void scanToPointCloud(const velodyne_msgs::VelodyneScan& velodyne_scan, 
      const velodyne::Calibration& calibration, pcl::PointCloud<PointT>& cloud) {

    // invoke callback for each packet
    for (unsigned i = 0; i < rawScan_->packets.size(); ++i) {
      processPacket(&rawScan_->packets[i]);
    }

  }
  
} /* velodyne */
