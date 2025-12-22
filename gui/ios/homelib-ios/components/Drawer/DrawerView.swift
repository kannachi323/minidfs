//
//  DrawerView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//

import SwiftUI

struct DrawerView : View {
    @Binding var isOpen : Bool
    
    var body : some View {
        
        VStack(alignment: .leading, spacing: 16) {
            HStack {
                DrawerItem(iconName: "folder", title: "My Folder") {
                    isOpen.toggle()
                }
            }
            HStack {
                DrawerItem(iconName: "folder", title: "My Folder") {
                    isOpen.toggle()
                }
            }
            HStack {
                DrawerItem(iconName: "folder", title: "My Folder") {
                    isOpen.toggle()
                }
            }
            
        }
        .padding(.top)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(.black)
    }
}

struct DrawerItem : View {
    var iconName: String
    var title : String
    var action: () -> Void
    
    var body : some View {
        Button(action: action) {
            HStack(spacing: 16) {
                Image(systemName: iconName)
                Text(title)
            }
        }
        .padding()
    }
}
