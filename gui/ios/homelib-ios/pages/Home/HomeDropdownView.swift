//
//  HomeDropdownView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//


import SwiftUI

struct HomeDropdownView: View {
    @State private var isOpen = false
    @State private var selectedSection: String = "Recents"

    let sections = ["Recents", "Frequently Accessed", "Suggested"]

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Dropdown bar (tappable)
            HStack {
                Text(selectedSection)
                    .font(.headline)
                    .foregroundColor(.primary)
                Spacer()
                Image(systemName: isOpen ? "chevron.up" : "chevron.down")
                    .foregroundColor(.gray)
            }
            .padding(10)
            .background(Color(.systemGray5))
            .cornerRadius(8)
            .onTapGesture {
                withAnimation {
                    isOpen.toggle()
                }
            }

            // Dropdown list
            if isOpen {
                VStack(alignment: .leading, spacing: 0) {
                    ForEach(sections, id: \.self) { section in
                        Text(section)
                            .padding(.vertical, 10)
                            .padding(.horizontal)
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .background(
                                selectedSection == section
                                    ? Color(.systemGray4)
                                    : Color(.systemGray6)
                            )
                            .onTapGesture {
                                withAnimation {
                                    selectedSection = section
                                    isOpen = false
                                }
                            }
                    }
                }
                .background(Color(.systemGray6))
                .cornerRadius(8)
                .shadow(radius: 3)
            }
        }
        .padding(.horizontal)
        .fixedSize()
    }
}
